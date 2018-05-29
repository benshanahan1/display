#include "Display.h"

static void dprint(FILE *stream, char *format, ...);

// Variables
pthread_mutex_t consoleLock;    // lock to avoid interleaving prints
va_list         args;           // variable argument function inputs
char            timestamp[32];  // store current time
time_t          rawtime;        // store raw time
struct tm      *timeinfo;       // store timeinfo struct
int             isLocked;       // is print mutex already locked (by user)?

// Default values
int   verbose      = ENABLE;   // Function verbosity
int   colorfulness = ENABLE;   // Function colorfulness
char  file[32]     = "?\0";    // store name of current file
int   autoNewline  = ENABLE;   // automatically add newline after printing
int   showTrace    = ENABLE;   // include function, file, timestamp with print

#ifdef MATLAB
int matlabMexPrintf = ENABLE;  // Display prints to terminal and Matlab console
#endif 

// Default stream array. Elements correspond to order of PrintType enum:
//    { STANDARD, WARNING, ERROR }
FILE *streams[3];

// Check if user is redirecting output to a text file.
int stdoutToFile = 0;
int stderrToFile = 0;



// Set Display verbosity. Verbosity is enabled by default.
int GetVerbose() { return verbose; }
int SetVerbose(int v)
{
    if (v == DISABLE || v == ENABLE)
        verbose = v;
    else
    {
        dprint(stderr, "ERROR: Invalid verbosity value.\n");
        exit(1);
    }
    return 0;
}


// Set colorfulness of text. ANSI colors are enabled by default.
int GetColorfulness() { return colorfulness; }
int SetColorfulness(int c)
{
    if (c == DISABLE || c == ENABLE)
        colorfulness = c;
    else
    {
        dprint(stderr, "ERROR: Invalid colorfulness value.\n");
        exit(1);
    }
    return 0;
}


// Set current filename.
char *GetFilename() { return file; }
int SetFilename(char *newFilename)
{
    if (newFilename != NULL)
    {
        strncpy(file, newFilename, sizeof(file)-1);
        return 0;
    }
    else
        return -1;
}


// Set automatic newline placed at end of Display print.
int GetAutoNewline() { return autoNewline; }
int SetAutoNewline(int a)
{
    if (a == ENABLE || a == DISABLE)
        autoNewline = a;
    else
    {
        dprint(stderr, "ERROR: Invalid auto newline value.\n");
        exit(1);
    }
    return 0;
}


// Set if we should print a trace with the Display() call or not.
int GetShowTrace() { return showTrace; }
int SetShowTrace(int s)
{
    if (s == ENABLE || s == DISABLE)
        showTrace = s;
    else
    {
        dprint(stderr, "ERROR: Invalid show trace value.\n");
        exit(1);
    }
    return 0;   
}


// Set stream (file descriptor) for output.
int SetStream(int streamType, FILE *newStream)
{
    if (streamType >= STANDARD && streamType <= ERROR)
        streams[streamType] = newStream;
    else
    {
        dprint(stderr, "ERROR: Invalid stream type. See PrintType enum.");
        exit(1);
    }
    return 0;
}
FILE *GetStream(int streamType)
{
    if (streamType >= STANDARD && streamType <= ERROR)
        return streams[streamType];
    else
        return NULL;
}


// Display print to Matlab console in addition to terminal. We only include 
// function for compilation if the -DMATLAB flag is included.
#ifdef MATLAB
int GetMatlabMexPrintf() { return matlabMexPrintf; }
int SetMatlabMexPrintf(int m)
{
    if (m == ENABLE || m == DISABLE)
        matlabMexPrintf = m;
    else
    {
        dprint(stderr, "ERROR: Invalid value, use ENABLE or DISABLE.\n");
        exit(1);
    }
    return 0;
}
#endif


// Block until we can lock the Display mutex.
int DisplayLock()
{
    if (!isLocked) pthread_mutex_lock(&consoleLock);
    isLocked = 1;
    return 0;
}

// Unlock the Display mutex.
int DisplayUnlock()
{
    if (isLocked) pthread_mutex_unlock(&consoleLock);
    isLocked = 0;
    return 0;
}



// InitializeDisplay sets the calling file's name.
int __InitializeDisplay(char *filename, int argc, char *argv[])
{
    // Check to see if the user is redirecting output ('>' symbol in bash). If
    // so, disable colorfulness so that the escape characters don't get in the
    // way.
    if (!isatty(fileno(stdout)))
        stdoutToFile = 1;
    if (!isatty(fileno(stderr)))
        stderrToFile = 1;

    // Override Defaults
    SetVerbose(ENABLE);
    SetColorfulness(ENABLE);
    SetStream(STANDARD, stdout);
    SetStream(WARNING, stderr);
    SetStream(ERROR, stderr);

    // Parse optional arguments.
    int opt;
    optind = 1;  // reset option index (parse all arguments again)
    opterr = 0;  // hide unknown option errors
    static struct option long_options[] =
    {
        {"silent",      no_argument,    0,  's'},
        {"no-color",    no_argument,    0,  'n'}
    };
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "sn", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'h' :
               // PrintHelp();
                break;
            case 's' :
                SetVerbose(DISABLE);
                break;
            case 'n' :
                SetColorfulness(DISABLE);
                break;
            default: break;
        }
    }
    
    // Get just the filename from full filepath.
    snprintf(file, sizeof(file)-1, "%s", basename(filename));
    return 0;
}

// Clean up Display, free memory, etc.
int CloseDisplay() { 
    pthread_mutex_destroy(&consoleLock);
    return 0; 
}



// Don't call this function. Use the Display(format, ...) macro instead!
void __Display(const char *function, int type, FILE *fd, char *color, \
    char *format, ...)
{

    // Lock debug printing to console so threads don't interleave. We only need
    // to do this if `isLocked` is 0. If it's 1, that means the user has chosen
    // to acquire the necessary mutex, so things are in their hands now...
    if (!isLocked) pthread_mutex_lock(&consoleLock);

    // Check if user is redirecting output to a text file.
    int old_colorfulness = GetColorfulness();
    if ( (stdoutToFile && (type == STANDARD)) || \
         (stderrToFile && ((type == WARNING) || (type == ERROR))) )
        SetColorfulness(DISABLE);

    // Get a pointer to our stream file descriptor.
    FILE *stream;
    if (type == CUSTOM)
        stream = fd;  // user-specified stream file descriptor
    else
        stream = GetStream(type);

    // Populate va_list with args from '...' parameter
    va_start(args, format);

    // Get current system timestamp
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%T", timeinfo);

    // Print message header to console and add a space at after it.
    if (showTrace)
    {
        if (colorfulness)
            dprint(stream, "%s[%s][%s][%s]", color, timestamp, file, function);
        else
            dprint(stream, "[%s][%s][%s]", timestamp, file, function);
    }

    if      (type == ERROR)   dprint(stream, "[ERROR] ");
    else if (type == WARNING) dprint(stream, "[WARNING] ");
    else if (showTrace)       dprint(stream, " ");

    // Print variable argument message body to console
    char tmpBuffer[BUFFLEN];
    vsnprintf(tmpBuffer, BUFFLEN-1, format, args);
    va_end(args);
    dprint(stream, tmpBuffer);

    // If colorfulness is enabled, reset the color after printing.
    if (colorfulness)
        dprint(stream, RESET);  // Reset ANSI color (RESET)
    
    if (autoNewline) dprint(stream, "\n");

    SetColorfulness(old_colorfulness);

    if (!isLocked) pthread_mutex_unlock(&consoleLock);

    return;
}



// Utility print function for internal use. Can append additional logic to all
// print calls in Display.c. This is useful in Matlab, for example, because an
// additional function call to mexPrintf() is required to display things in the
// Matlab command window when running the GUI.
static void dprint(FILE *stream, char *format, ...)
{
    char    messageBuffer[BUFFLEN];
    va_list args;

    // Insert args into format string and buffer it.
    va_start(args, format);
    vsnprintf(messageBuffer, BUFFLEN-1, format, args);
    va_end(args);

    // NOTE:    When building with SWIG for Python, the file descriptors in 
    //          Python are not analogous to those used by C, etc. Therefore,
    //          you must replaced fprintf with just printf (no file descriptor)
    //          and to do this, you must include the -DNOFPRINTF compiler flag
    //          when building. This will produce a segmentation fault 
    //          (segfault) otherwise.
    #ifdef NOFPRINTF
        printf("%s", messageBuffer);
    #else
        fprintf(stream, "%s", messageBuffer);
    #endif

    // Print to terminal and Matlab command window (if required).
    #ifdef MATLAB
    if (stream == stdout || stream == stderr)
        if (matlabMexPrintf)
            mexPrintf(messageBuffer);
    #endif

    return;
}
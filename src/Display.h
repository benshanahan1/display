/**
 * @file
 * @brief Traceable printing to console and files!
 *
 * The Display library allows a user to traceably print to the console from any
 * process. Display supports ANSI colors, allowing the user to visually change
 * console output to better alert the user.
 *
 * Example usage (`TestProcess.c`):
 *
 *      @code
 *      #include "Display.h"
 *
 *      int main(int argc, char *argv[])
 *      {
 *          InitializeDisplay(argc, argv);
 *
 *          Display("Hello, %s!", "World");
 *          DisplayError("This is an error message.");
 *          DisplayColor(MAGENTA, "I am %d years old.", 22);
 *
 *          CloseDisplay();
 *
 *          return 0;
 *      }
 *      @endcode
 *
 *
 *
 * By default, processes that use Display will boot with verbosity enabled. To 
 * disable all non-error printing to console, run the process in silent mode
 * with the `--silent` or `-s` flag. For example:
 *
 *      $ ./TestProcess --silent
 *      $ ./TestProcess -s
 *
 * To manually disable all ANSI colors from printing with Display, use the 
 * `--no-color` or `-n` override flag when launching the process. For example:
 *
 *      $ ./TestProcess --no-color
 *      $ ./TestProcess -n
 *
 *
 *
 * To compile Display for Matlab, include the -DMATLAB compiler flag 
 * immediately following the compiler name (i.e. `gcc` or `mex`). For example:
 *
 *      $ mex -DMATLAB MatlabProcess.c Display.c -o MatlabProcess
 *
 *
 * To compile Display for a system that cannot handle C file descriptors, for
 * example, Python, use the -DNOFPRINTF compiler flag following the compiler
 * name.
 *
 *
 * When redirecting output from Display (via '>' bash character), Display is 
 * smart enough to detect this and will disable colorfulness for you 
 * automatically so that there are no escape characters in your output files.
 * For example:
 *
 *      $ ./MyProcess > everythingButErrors.txt
 *      $ ./MyProcess 2> onlyErrors.txt
 *      $ ./MyProcess &> everything.txt
 *      $ ./MyProcess 1>log.txt 2>errors.txt 
 *
 *
 *
 * @note If you see a question mark (?) where the filename should be, you
 *       must initialize the Display library before calling Display. To do
 *       this, call `InitializeDisplay(argc, argv)`.
 *
 *
 *
 * @authors Benjamin Shanahan
 */

#ifndef __ESPA_DISPLAY_UTILITY__
#define __ESPA_DISPLAY_UTILITY__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>  // basename()
#include <unistd.h>

#ifdef MATLAB
#include "mex.h"
#endif

#define BUFFLEN 256  ///< Buffer length for strings being printed.



/* Define ANSI colors for printing to console.
 * 
 * More information:
 *    https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
 */
#define BLACK           "\x1b[30m"          ///< ANSI color code for black.
#define RED             "\x1b[31m"          ///< ANSI color code for red.
#define GREEN           "\x1b[32m"          ///< ANSI color code for green.
#define YELLOW          "\x1b[33m"          ///< ANSI color code for yellow.
#define BLUE            "\x1b[34m"          ///< ANSI color code for blue.
#define MAGENTA         "\x1b[35m"          ///< ANSI color code for magenta.
#define CYAN            "\x1b[36m"          ///< ANSI color code for cyan.
#define WHITE           "\x1b[37m"          ///< ANSI color code for white.
#define RESET           "\x1b[0m"           ///< Reset to no color.

/* Additionally ANSI codes for text formatting. */
#define BOLD            "\x1b[1m"           ///< ANSI code for bolded text.
#define FAINT           "\x1b[2m"           ///< ANSI code for faint text.
#define ITALIC          "\x1b[3m"           ///< ANSI code for italicized text.
#define UNDERLINE       "\x1b[4m"           ///< ANSI code for underlined text.

/** Enumerated values for setting Display verbosity. */
enum Verbosity {
    DISABLE,  ///< Disable verbosity.
    ENABLE    ///< Enable verbosity.
};

/** Enumerated output types for determining what user is printing. */
enum PrintType {
    STANDARD,  ///< Standard print, obeys defined verbosity.
    WARNING,   ///< Warning (yellow), prints regardless of verbosity.
    ERROR,     ///< Error (red), prints regardless of verbosity.
    
    /** User will specify file descriptor to print to.
      * @note This is not a valid option for SetStream(). */
    CUSTOM
};



/** Set Display verbosity. Verbose is enabled by default. */
int SetVerbose(int v);
/** Return value of `verbose`. */
int GetVerbose();
int verbose;  ///< Use Set/GetVerbose() to access this value.

/**
 * Enable or disable ANSI text coloring. This option is included for operating
 * systems or terminals that do not support ANSI color formatting. Colorfulness
 * is enabled by default.
 */
int SetColorfulness(int c);
/** Return value of `colorfulness`. */
int GetColorfulness();
int colorfulness;  ///< Use Set/GetColorfulness() to access this value.

/** Set filename used in Display trace. Provided for manual override. */
int SetFilename(char *newFilename);
/** Return value of filename used in Display trace. */
char *GetFilename();

/** Set automatic newline inclusion after print. Enabled by default. */
int SetAutoNewline(int a);
/** Get automatic newline inclusion setting. */
int GetAutoNewline();

/** 
 * Set show trace (include function name and timestamp). Enabled by default. 
 * @note By disabling the trace, you are also removing ANSI color information,
 *       so prints can no longer be colorful.
 */
int SetShowTrace(int s);
/** Get show trace setting. */
int GetShowTrace();

/**
 * Set destination stream (file descriptor). For example, the default is 
 * `stdout` for STANDARD printing, and `stderr` for WARNINGs and ERRORs. 
 * However, you could also specify a text file for logging to disk, for 
 * example. `streamType` is defined in the `PrintType` enum.
 *
 * Change the WARNING stream so that all warnings are sent to `stdout` instead
 * of to `stderr` (Display sends WARNINGs to `stderr` by default).
 *      @code
 *      SetStream(WARNING, stdout);
 *      @endcode
 *
 * Change the STANDARD stream so that all output redirects into a text file
 * (logfile.txt) instead of to `stdout` (the terminal).
 *      @code
 *      FILE *fd = open("logfile.txt", "w");
 *      SetStream(STANDARD, fd);
 *      @endcode
 */
int SetStream(int streamType, FILE *newStream);
/** 
 * Return stream file descriptor. Stream `streamType` can be one of the 
 * enumerated options in the `PrintType` enum.
 */
FILE *GetStream(int streamType);
FILE *stream;  ///< Use Set/GetStream() to access this file descriptor.

/**
 * Enable or disable Matlab mexPrintf printing when using Display in Matlab.
 * This option is enabled by default when compiled Display for Matlab, but can
 * interfere with printing when running Matlab in `-nodisplay` mode. In this
 * mode, use `SetMatlabMexPrintf(DISABLE)` to prevent double printing to the
 * Matlab command window.
 *
 * @note You need to compile Display using the `-DMATLAB` flag in order to use
 *       this functionality. This flag must be included immediately following
 *       the compiler name (i.e. `gcc` or `mex`).
 */
int SetMatlabMexPrintf(int m);
/** Get Matlab mexPrintf setting (either enabled or disabled). */
int GetMatlabMexPrintf();



/** 
 * Acquire mutex lock for printing to console. If for some reason you want to
 * manage your own Display mutex, this function lets you acquire the console
 * print lock.
 */
int DisplayLock();
/** Release mutex lock for printing to console. */
int DisplayUnlock();



/**
 * Initialize the Display utility in the current process. This function must be
 * called in any process where you wish to use the Display utility (prior to
 * calls to `Display()`).
 *
 * This is required because we need to figure out what the calling filename is,
 * and the only way to do that is through the `__FILE__` compiler macro.
 */
#define InitializeDisplay(argc, argv) __InitializeDisplay(__FILE__, argc, argv);

/**
 * Display arbitrary text to console, including timestamp, file, and function
 * of origin.
 *
 * @param[in] format String containing formatting characters.
 * @param[in] ...    Variable number of format arguments.
 *
 * Example usage (called in function `FunctionName()` in file FileName.c):
 *
 *      @code
 *      Display("Hello, %s!", "Ben");
 *      @endcode
 *
 * Example console output:
 *
 *      @code
 *      [12:00:00][FileName][FunctionName] Hello, Ben!
 *      @endcode
 */
#define Display(format, ...) \
    if (verbose) \
        __Display(__FUNCTION__, STANDARD, NULL, RESET, format, ##__VA_ARGS__)
//
// Additional note for the above macro: the two ##'s preceding the __VA_ARGS__
// are GCC-specific and allow zero variadic inputs in a macro. Without the ##
// included, __VA_ARGS__ leaves a trailing comma when no arguments are 
// supplied, causing the macro to result in compilation errors.
//
// For more information, see: 
//     https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html

/** 
 * Print a warning to console regardless of verbosity.
 *
 *      @code
 *      DisplayWarning("This is a warning!");
 *      @endcode
 */
#define DisplayWarning(format, ...) \
    __Display(__FUNCTION__, WARNING, NULL, BOLD YELLOW, format, ##__VA_ARGS__)

/**
 * Print an error to console regardless of verbosity.
 *
 *      @code
 *      DisplayError("This is an error!");
 *      @endcode
 */
#define DisplayError(format, ...) \
    __Display(__FUNCTION__, ERROR, NULL, BOLD RED, format, ##__VA_ARGS__)

/** 
 * Print in a custom color.
 * 
 * Example usage:
 *
 *      @code
 *      DisplayColor(CYAN, "x = %d", 76);
 *      @endcode
 *
 * Color codes are defined in this header file. Additional color codes can be
 * found <a href="https://en.wikipedia.org/wiki/ANSI_escape_code#Colors">here</a>.
 */
#define DisplayColor(color, format, ...) \
    if (verbose) \
        __Display(__FUNCTION__, STANDARD, NULL, color, format, ##__VA_ARGS__)

/**
 * Print directly to a file descriptor.
 *
 * @note Since Display uses ANSI colors by default, if you write to a file, you
 *       may see the color codes and weird characters. To hide these, use 
 *       `SetColorfulness(DISABLE)`.
 *
 * Important: DisplayFile() disregards verbosity, so it will always print to a
 * file if it is called.
 *
 * Example usage:
 *
 *      @code
 *      SetColorfulness(DISABLE);
 *      FILE *logFile = open("logFile.txt","w");
 *      DisplayFile(logFile, "Process booted (%d).", 6);
 *      @endcode
 */
#define DisplayFile(fd, format, ...) \
    __Display(__FUNCTION__, CUSTOM, fd, RESET, format, ##__VA_ARGS__)



/**
 * Close the Display utility after use. Should be called at the end of 
 * execution of a process.
 */
int CloseDisplay();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/** Do not call this function, use `InitializeDisplay()` instead. */
int __InitializeDisplay(char *filenameRaw, int argc, char *argv[]);

/** Do not call this function, use `Display()` instead. */
void __Display(const char *function, int type, FILE *fd, char *color, \
    char *format, ...);

#endif  // end of include guard
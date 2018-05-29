#include "Display.h"

int main(int argc, char *argv[])
{
    InitializeDisplay(argc, argv);

    // This will only print if verbosity is enabled (-v flag).
    Display("This is a number! %d", 5);
    Display("Nothing to format, just text!");

    // This will print in red regardless of process verbosity.
    DisplayError("Welp, this is an error (%s)!", "ignore verbosity");
    DisplayError("Another error :(");

    // This will print in yellow regardless of process verbosity.
    DisplayWarning("This is a warning!");
    DisplayWarning("Numbers: %d, %d, %d", 1, 2, 3);

    // This will print in a custom color (obeying verbosity). Notice how we can
    // combine difference ANSI color / format codes together. The C 
    // preprocessor concatenates the strings automatically for us!
    DisplayColor(ITALIC CYAN, "This is a custom color print message!");
    DisplayColor(BOLD FAINT GREEN, "Hello, %s!", "Ben");

    // Check if colorfulness flag is working.
    SetColorfulness(DISABLE);
    DisplayColor(GREEN, "This text should be in green, but colorfulness is disabled!");

    SetAutoNewline(DISABLE);
    Display("Hello, ");
    SetShowTrace(DISABLE);
    SetAutoNewline(ENABLE);
    Display("World!");
    
    DisplayLock();
    Display("Here is a print without a trace.");
    DisplayError("Here is an error print without a trace.");
    DisplayUnlock();

    SetShowTrace(ENABLE);

    FILE *fd = fopen("testOutput.txt","w");
    SetStream(STANDARD, fd);
    Display("Hello, text file!");
    Display("The number five: %d", 5);
    SetStream(STANDARD, stdout);
    Display("Wrote to output text file, `testOutput.txt`.");

    DisplayFile(fd, "Another line in the same open %s!", "file");
    fclose(fd);

    CloseDisplay();
    return 0;
}

# Display

Lightweight C shared library for tracing to console.



## Features

* Simple trace (it's trivial to see from which process/function the Display print was called, as well as what time it was called)
* Colorful! Specify and use different colors for different situations (errors, warnings, etc.)
* Uses macros to make initialization easier.
* Build documentation with Doxygen.



## Example process using Display

```C
#include "Display.h"

int main(int argc, char *argv[])
{
    InitializeDisplay(argc, argv);

    Display("Hello, %s!", "World");
    DisplayError("This is an error message.");
    DisplayColor(MAGENTA, "I am %d years old.", 22);

    CloseDisplay();

    return 0;
}
```


Check out the full feature demo file in the demo/ directory. Build it from the command line with `make`.



## Documentation

Documentation uses Doxygen. Install Doxygen on Linux like so:

    sudo apt install python-pydot python-pydot-ng graphviz doxygen

Once installed, build the documentation:

    cd docs
    make

The built documentation index is placed at 'docs/html/index.html'. Open this in your default web browser.
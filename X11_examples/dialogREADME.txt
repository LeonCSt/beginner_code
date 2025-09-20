Notes about leon.h, leon_main.cpp and leon_dialog.cpp

I wanted a separate cpp file to handle all dialog requirements.
A typical application may need 10 different dialogs, and all these
can be handled by a single cpp, by using mode switching code.
All calling variables and return variables can be shared via the
associated header file.

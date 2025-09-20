/* leon.h: header file for leon_main.cpp and leon_dialog.cpp
        these 3 files reside in the same directory       [250418]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern Display *dis;
extern GC gc;
extern Window win;
extern Atom WM_DELETE_WINDOW;
extern XFontStruct *font;
extern bool reply;
extern int mx, my;       // mouse position

void maind();

/* leon060.h: header file for leon061.cpp and leon062.cpp
        these 3 files reside in the same directory       [260524]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <thread>
#include <chrono>
#include <stdio.h>

extern Display *dis;
extern GC gc;
extern Window win;
extern Atom WM_DELETE_WINDOW;
extern XShmSegmentInfo shminfo;
extern FT_Library lbrry;
extern FT_Face face;
extern Cursor hndcursor;
extern unsigned *p;              // pixels
extern bool reply;
extern int txthght, mX, mY, bufW, bufH;

void maind();

int text_width(unsigned char* str, int length);
        /*  unsigned char *str | pointer to start in utf8 byte array
            int length         | length of encoding to read  */

void text_run(unsigned *m, int bfW, int bfH, int X, int Y,
        unsigned char *str, int length, unsigned fg);
        /*  unsigned *m        | pointer to beginning of buffer pixels
            int bfW, bfH       | buffer width and height
            int X, Y           | start position
            unsigned char *str | pointer to start in utf8 byte array
            int length         | length of encoding to read
            unsigned fg, bg    | foreground and background colors   */

void fill_rectangle(unsigned *m, int bfW, int bfH, int X, int Y,
        int width, int height, unsigned fg);
        /*  unsigned *m        | pointer to beginning of buffer pixels
            int bfW, bfH       | buffer width and height
            int X, Y           | topleft position
            int width, height  | size
            unsigned fg        | color  */

void draw_rectangle(unsigned *m, int bfW, int bfH, int X, int Y,
        int width, int height, unsigned fg);



/*  leon020.cpp: putting an image onto a window
    create an XImage, modify then paste on window
    g++ leon020.cpp -o leon -lX11                        [241125]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <math.h>
#include <string>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XImage *img;
union mix_t {
  char c[112896];
  unsigned i[28224];
} u;

void draw() {
  int i, j, k, m, n;
  for (i = 0; i < 28224; i++) u.i[i] = 0xff332d20;
  double r;
  unsigned c = 0xff99bbff;
  k = 14224; r = 58.0;
  for (i = 0; i < 42; i++) {
    j = cos(asin(i / r)) * r + 0.5;
    n = i * 224; m = j * 224;
    u.i[k + i + m] = c; u.i[k + i - m] = c;
    u.i[k - i + m] = c; u.i[k - i - m] = c;
    u.i[k + j + n] = c; u.i[k + j - n] = c;
    u.i[k - j + n] = c; u.i[k - j - n] = c;
  }
}

void paint() {
  XClearWindow(dis, win);
  XPutImage(dis, win, gc, img, 0, 0, 320, 180, 224, 126);
  XFlush(dis);
}

void init() {
  dis = XOpenDisplay(0);
  gc = XDefaultGC(dis, DefaultScreen(dis));
  XSetForeground(dis, gc, 0xffaaaa66);
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = StructureNotifyMask | ButtonPressMask |
          KeyReleaseMask | ExposureMask;
  int left = (XDisplayWidth(dis, 0) - 864) / 2;
  int top = (XDisplayHeight(dis, 0) - 486) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          864, 486, 1, DefaultDepth(dis,0), InputOutput,
	  DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 864; sh.height = 486;
  sh.min_width = 432; sh.min_height = 243;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  char snm[] = "Test GUI(Title Bar Text)";
  char *psnm = snm;
  XStringListToTextProperty(&psnm, 1, &nm);
  char sicnm[] = "Test GUI(Icon Text)";
  char *psicnm = sicnm;
  XStringListToTextProperty(&psicnm, 1, &icnm);
  XSetWMProperties(dis, win, &nm, &icnm, NULL, 0, &sh, &hnts, NULL);
  XMapWindow(dis, win);
}

int main() {
  init();
  img = XCreateImage(dis, DefaultVisual(dis, 0), DefaultDepth(dis, 0),
          ZPixmap, 0, &u.c[0], 224, 126, 32, 0);
  draw();
  KeySym key;
  char text;
  bool f[2]; // f[0] event while loop keeper, f[1] paint flag
  f[0] = true;
  XEvent evnt;
  while(f[0]) {
    f[1] = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case Expose:
        f[1] = true; break;
      case ButtonPress:
        f[1] = true; break;
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_c) XClearWindow(dis, win);
        else if ((key == XK_Escape) || (key == XK_q)) f[0] = false;
        else printf("You pressed the %c key!\n",text);
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                f[0] = false;
        break;
    }
    if (f[1]) paint();
  }
  img->data = NULL;
  XDestroyImage(img);
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
}


/* leon030.cpp:  adding an icon to your app
// make an icon for your window via a Pixmap, and a Pixmap mask.
// g++ leon030.cpp -o leon -lX11                         [241128]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XImage *img, *mvw;
Pixmap xmp, msk;
union pnts_t {
  short p[56] = {47, 5, 56, 29, 65, 50, 75, 68, 64, 64, 80, 76, 90, 90,
          77, 83, 65, 79, 56, 78, 58, 71, 58, 65, 56, 59, 50, 56,
          44, 56, 39, 59, 37, 65, 37, 71, 39, 78, 30, 79, 18, 83,
          5, 90, 16, 70, 28, 49, 36, 30, 46, 35, 38, 26, 47, 5};
  XPoint xp[28];
} pnts;
XPoint *pxp = pnts.xp;

void microview() {
  int wy = 10, wx;
  for (int y = 0; y < 24; y++) {
    wx = 10;
    for (int x = 0; x < 16; x++) {
      XSetForeground(dis, gc, XGetPixel(mvw, x, y));
      XFillRectangle(dis, win, gc, wx, wy, 13, 13);
      wx += 14;
    }
    wy += 14;
  }
  XSetForeground(dis, gc, 0xffaaaa66);
  XFlush(dis);
}

void mkicon() {
  xmp = XCreatePixmap(dis, win, 96, 96, 24);
  XSetForeground(dis, gc, 0xff3a99ee);
  XFillPolygon(dis, xmp, gc, pxp, 28, Nonconvex, CoordModeOrigin);
  img = XGetImage(dis, xmp, 0, 0, 96, 96, AllPlanes, ZPixmap);
  msk = XCreatePixmap(dis, win, 96, 96, 1);
  XGCValues tgcv;
  tgcv.function = GCFunction; tgcv.foreground = 1; tgcv.background = 0;
  GC tmpgc = XCreateGC(dis, msk, GCForeground | GCBackground, &tgcv);
  XFillPolygon(dis, msk, tmpgc, pxp, 28, Nonconvex, CoordModeOrigin);
  XFreeGC(dis, tmpgc);
  XSetForeground(dis, gc, 0xffaaaa66);
}

void paint() {
  XClearWindow(dis, win);
  XPutImage(dis, win, gc, img, 0, 0, 272, 192, 96, 96);
  XFlush(dis);
}

void init() {
  dis = XOpenDisplay(0);
  gc = XDefaultGC(dis, DefaultScreen(dis));
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = KeyReleaseMask | ButtonPressMask |
	  PointerMotionMask | ExposureMask | StructureNotifyMask;
  int left = (XDisplayWidth(dis, 0) - 642) / 2;
  int top = (XDisplayHeight(dis, 0) - 480) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          640, 480, 1, DefaultDepth(dis,0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  mkicon();
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 640; sh.height = 480;
  sh.min_width = 320; sh.min_height = 240;
  sh.win_gravity = CenterGravity;
  hnts.flags = 37; hnts.input = true;
  hnts.icon_pixmap = xmp; hnts.icon_mask = msk;
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
  KeySym key;
  char text;
  bool f[2]; // f[0] event while loop keeper, f[1] paint flag
  int mx, my;
  f[0] = true;
  XEvent evnt;
  while(f[0]) {
    f[1] = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_c) XClearWindow(dis, win);
        else if ((key == XK_Escape) || (key == XK_q)) f[0] = false;
        else printf("You pressed the %c key!\n",  text);
        break;
      case ButtonPress:
        f[1] = true; break;
      case MotionNotify:
        mx = evnt.xbutton.x_root ; my = evnt.xbutton.y_root;
        if (mx < 8) mx = 8;
        else if (mx > (XDisplayWidth(dis, 0) - 8))
                mx = XDisplayWidth(dis, 0) - 8;
        if (my < 12) my = 12;
        else if (my > (XDisplayHeight(dis, 0) - 12))
                my = XDisplayHeight(dis, 0) - 12;
        mvw = XGetImage(dis, XDefaultRootWindow(dis),
                mx - 8, my - 12, 16, 24, AllPlanes, ZPixmap);
        microview();
        break;
      case Expose:
        f[1] = true; break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                f[0] = false;
        break;
    }
    if (f[1]) paint();
  }
  XFreePixmap(dis, xmp); XFreePixmap(dis, msk);
  img->data = NULL;
  XDestroyImage(mvw);
  XDestroyImage(img);
  XDestroyWindow(dis, win);
}

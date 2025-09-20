/*  leon040.cpp: looking at legacy fonts
    text and font explorations
    mouseclick to toggle pointer grab (for upclose viewing pixels
    outside your client)
    g++ leon040.cpp -o leon -lX11                         [241217]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XFontStruct *font;
XImage *img;

void prnt() {
  string str = "-xos4-terminus-bold-r-*--24*\n"
          "-misc-fixed*\n"
          "-dec-terminal-bold-r-*--14-*-*-*-*-*-iso*\n"
          "-dec-terminal-bold-r-*--14-*-*-*-*-*-dec*\n"
          "-bitstream-terminal-medium-r-*--18-*-*-*-*-*-iso*\n"
          "-bitstream-terminal-medium-r-*--18-*-*-*-*-*-dec*\n"
          "-*-charter-medium-r-*--25*\n"
          "-*-lucida-medium-r-*-*-25*\n"
          "-*-lucidabright-medium-r-*--25*\n"
          "-*-lucidatypewriter-medium-*-*-*-25*\n"
          "-*-courier-bold-r-*--25*\n"
          "-*-helvetica-medium-r-*--25*\n"
          "-*-new century schoolbook-medium-r-*--25*\n"
          "-*-symbol-*-*-*--25*\n"
          "-*-times-medium-r-*--25*\n"
          "-*-utopia-regular-r-*--25*\n";
  string pttn, xlfd;
  int Ycoord = 30, pos = 0, nxt = 0, num = 0, end = str.size();
  while (pos < end) {
    nxt = str.find("\n", pos);
    pttn = str.substr(pos, nxt - pos);
    char **list = XListFonts(dis, pttn.c_str(), 20, &num);
    printf("num = %d\n", num);
    if (num) {
      for (int i = 0; i < num; i++) {
        xlfd = *(list + i);
        printf(xlfd.c_str()); printf("\n");
      }
      xlfd  = *(list);
      font = XLoadQueryFont(dis, *(list));
      if (font) {
        XSetFont(dis, gc, font->fid);
        XDrawString(dis, win, gc, 20, Ycoord, xlfd.c_str(),
                xlfd.size());
        XDrawString(dis, win, gc, 60, Ycoord + 25,
                "the quick brown fox jumps over the lazy dog", 43 );
      }
      else printf("No success");
      XFreeFontNames(list);
    }
    pos = nxt  + 1;
    Ycoord += 55;
  }
}

void microview() {
  int wy = 564, wx;
  for (int y = 0; y < 24; y++) {
    wx = 976;
    for (int x = 0; x < 16; x++) {
      XSetForeground(dis, gc, XGetPixel(img, x, y));
      XFillRectangle(dis, win, gc, wx, wy, 13, 13);
      wx += 14;
    }
    wy += 14;
  }
  XSetForeground(dis, gc, 0xffaaaa66);
  XFlush(dis);
}

void paint() {
  XClearWindow(dis, win);
  prnt();
  XFlush(dis);
}

void init() {
  dis = XOpenDisplay(0);
  gc = XDefaultGC(dis, DefaultScreen(dis));
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = StructureNotifyMask | PointerMotionMask |
          ButtonPressMask | KeyReleaseMask | ExposureMask;
  int left = (XDisplayWidth(dis, 0) - 1200) / 2;
  int top = (XDisplayHeight(dis, 0) - 900) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          1200, 900, 1, DefaultDepth(dis,0), InputOutput,
	  DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 1200; sh.height = 900;
  sh.min_width = 600; sh.min_height = 450;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  char snm[] = "Test GUI(Title Bar Text)";
  char *psnm = snm;
  XStringListToTextProperty(&psnm, 1, &nm);
  char sicnm[] = "Test GUI(Icon Text)";
  char *psicnm = sicnm;
  XStringListToTextProperty(&psicnm, 1, &icnm);
  XSetWMProperties(dis, win, &nm, &icnm, NULL, 0, &sh, &hnts, NULL);
  XSetForeground(dis, gc, 0xffaaaa66);
  XMapWindow(dis, win);
}

int main() {
  init();
  KeySym key;
  char text;
  bool f[3]; // f[0] event while loop keeper, f[1] paint flag,
             // f[2] pointer is grabbed
  int mx, my;
  f[0] = true; f[2] = false;
  XEvent evnt;
  while(f[0]) {
    f[1] = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case Expose:
        f[1] = true; break;
      case MotionNotify:
        mx = evnt.xbutton.x_root ; my = evnt.xbutton.y_root;
        if (mx < 8) mx = 8;
        else if (mx > (XDisplayWidth(dis, 0) - 8))
                mx = XDisplayWidth(dis, 0) - 8;
        if (my < 12) my = 12;
        else if (my > (XDisplayHeight(dis, 0) - 12))
                my = XDisplayHeight(dis, 0) - 12;
        img = XGetImage(dis, XDefaultRootWindow(dis),
                mx - 8, my - 12, 16, 24, AllPlanes, ZPixmap);
        microview();
        break;
      case ButtonPress:
        if (f[2]) {
          XUngrabPointer(dis, CurrentTime);
          f[2] = false;
        }
        else {
          XGrabPointer(dis, XDefaultRootWindow(dis) , false,
                  ButtonPressMask | PointerMotionMask,
                  GrabModeAsync, GrabModeAsync, None, None,
		  CurrentTime);
          f[2] = true;
        }
        break;
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
  XUngrabPointer(dis, CurrentTime);
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
}

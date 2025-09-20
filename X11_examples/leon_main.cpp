/*  leon_main.cpp:  launching a pop-up dialog box
    Calling  maind();  launches the dialog box
    Size of parent is fixed when child is mapped.
    g++ leon_main.cpp leon_dialog.cpp -o leon -lX11      [250418]  */

#include "leon.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string>
using namespace std;

// defining extern variables declared in leon.h
Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XFontStruct *font;
bool reply;
int mx, my;                      // mouse position
// end extern variables

XSizeHints sh;
int btnleft, btntop, btnwidth, btnheight;     // button rectangle
string btnlabel = "Click to open pop-up dialog";
bool bf;      // button has focus

void launchd() {
  sh.min_width = sh.width; sh.min_height = sh.height;
  sh.max_width = sh.width; sh.max_height = sh.height;
  XSetWMNormalHints(dis, win, &sh);
  maind();
}

void buttn() {
  if (bf) {
    XSetForeground(dis, gc, 0xff222277);
    XFillRectangle(dis, win, gc, btnleft + 3, btntop + 3,
            btnwidth - 6, btnheight - 6);
    XSetForeground(dis, gc, 0xffaaaa66);
  }
  XDrawRectangle(dis, win, gc, btnleft, btntop, btnwidth, btnheight);
  XDrawString(dis, win, gc, btnleft + 5, btntop + 30,
          btnlabel.c_str(), btnlabel.size());
}

void paint() {
  XClearWindow(dis, win);
  buttn();
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
  int left = (XDisplayWidth(dis, 0) - 800) / 2;
  int top = (XDisplayHeight(dis, 0) - 600) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          800, 600, 1, DefaultDepth(dis, 0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  sh.flags = PPosition | PSize | PMinSize | PMaxSize |  PWinGravity;
  sh.x = left; sh.y = top; sh.width = 800; sh.height = 600;
  sh.min_width = 800; sh.min_height = 600;
  sh.max_width = XDisplayWidth(dis, 0) - 30;
  sh.max_height = XDisplayHeight(dis, 0) - 100;
  sh.win_gravity = CenterGravity;
  XWMHints hnts;
  hnts.flags = 1; hnts.input = true;
  XTextProperty nm, icnm;
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
  bool loopy,  pflag;		// event loop keeper and paint flag
  loopy = true;
  string pttn ="-*-lucida-medium-r-*-*-25*";
  font = XLoadQueryFont(dis, pttn.c_str());
  if (font) {
    XSetFont(dis, gc, font->fid);
    btnleft = 295; btntop = 170;
    btnwidth = XTextWidth(font, btnlabel.c_str(),
            btnlabel.size()) + 10;
    btnheight = 40;
  }
  else {
    printf("pattern failed\n");
    loopy = false;
  }
  XEvent evnt;
  XConfigureEvent *xcfg;
  while(loopy) {
    pflag = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Tab) {
          bf = !bf;
          pflag = true;
        }
        else if ((key == XK_Return) && (bf)) launchd();
        else if ((key == XK_Escape) || (key == XK_q)) loopy = false;
        break;
      case ButtonPress:
        if (bf) launchd();
        break;
      case MotionNotify:
        mx = evnt.xbutton.x; my = evnt.xbutton.y;
        if (bf) {
          if ((mx < btnleft) || (mx > (btnleft + btnwidth)) ||
                  (my < btntop) || (my > (btntop + btnheight))) {
            bf = false; pflag = true;
          }
        }
        else if ((mx > btnleft) && (mx < (btnleft + btnwidth)) &&
                (my > btntop) && (my < (btntop + btnheight))) {
          bf = true; pflag = true;
        }
        break;
      case Expose:
        pflag = true; break;
      case ConfigureNotify:
        xcfg = (XConfigureEvent *)&evnt;
        sh.width = xcfg->width; sh.height = xcfg->height;
        break;
      case DestroyNotify:
        sh.min_width = 800; sh.min_height = 600;
        sh.max_width = XDisplayWidth(dis, 0) - 30;
        sh.max_height = XDisplayHeight(dis, 0) - 100;
        XSetWMNormalHints(dis, win, &sh);
        if (reply) printf("Reply = 'On'\n");
        else printf("Reply = 'Off'\n");
        XSetForeground(dis, gc, 0xffaaaa66);
        bf = false; pflag = true;
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loopy = false;
        break;
    }
    if (pflag) paint();
  }
  XDestroyWindow(dis, win);
}


/* leon010.cpp: Window shows with inputs, resizes and animation
   mouseclick to move text, arrow keys to move rectangle,
   [Q] or [Esc] to close
   g++ leon010.cpp -o leon -lX11                      [241115]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <thread>
#include <chrono>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
int mX, mY, rectX, rectY;
bool k[6]; // arrow key flags

void paint() {
  XClearWindow(dis, win);
  XFillRectangle(dis, win, gc, rectX, rectY, 50, 80);
  XDrawString(dis, win, gc, mX, mY, "SomeText", 8);
  XFlush(dis);
}

void animate() {
  k[5] = true;
  while (k[4]) {
    if (k[0]) rectX += 5;
    if (k[1]) rectX -= 5;
    if (k[2]) rectY += 5;
    if (k[3]) rectY -= 5;
    paint();
    this_thread::sleep_for(chrono::milliseconds(30));
  }
  k[5] = false;
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
          KeyPressMask | KeyReleaseMask | ExposureMask;
  int left = (XDisplayWidth(dis, 0) - 642) / 2;
  int top = (XDisplayHeight(dis, 0) - 480) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          640, 480, 1, CopyFromParent, CopyFromParent,
          CopyFromParent, attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 640; sh.height = 480;
  sh.min_width = 320; sh.min_height = 240;
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
  XkbSetDetectableAutoRepeat(dis, true, NULL);
}

int main() {
  init();
  KeySym key;
  char text;
  mX = 10; mY = 10; rectX = 100; rectY = 100;
  bool f[2]; // f[0] event while loop keeper, f[1] paint flag
  f[0] = true;
  XEvent evnt;
  while(f[0]) {
    f[1] = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case Expose:
        f[1] = true; break;
      case KeyPress:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Right) k[0] = true;
        else if (key == XK_Left) k[1] = true;
        else if (key == XK_Down) k[2] = true;
        else if (key == XK_Up) k[3] = true;
        break;
      case ButtonPress:
        f[1] = true;
        mX = evnt.xbutton.x; mY = evnt.xbutton.y; break;
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_c) XClearWindow(dis, win);
        else if ((key == XK_Escape) || (key == XK_q)) f[0] = false;
        else if (key == XK_Right) k[0] = false;
        else if (key == XK_Left) k[1] = false;
        else if (key == XK_Down) k[2] = false;
        else if (key == XK_Up) k[3] = false;
        else printf("You pressed the %c key!\n", text);
        break;
      case ClientMessage:
        if((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                f[0] = false;
        break;
    }
    if (!k[4] && (k[0] || k[1] || k[2] || k[3])) {
      k[4] = true;
      if (!k[5]) { thread t1(animate); t1.detach(); }
    }
    else if (k[4] && (!k[0] && !k[1] && !k[2] && !k[3])) k[4] = false;
    if (f[1]) paint();
  }
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
}

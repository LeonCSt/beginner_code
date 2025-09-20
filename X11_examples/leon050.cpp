/*  leon050.cpp:  modern fonts with Xft text rendering
    to see listing in terminal : -    
         fc-list | grep -i "\.otf"  or  "\.ttf"
    mouseclick to toggle pointergrab
    g++ leon050.cpp -o leon -lX11 -lXft -lfontconfig -I/usr/include/freetype2
                                                        [250305]   */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <string>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XftFont *font;
XftDraw *draw;
XRenderColor clr;
XftColor colr;
XImage *img;

void prnt() {
  char strfont[272];
  char *dest = strfont;
  FcPattern *temp, *patt;
            //Section:  creating a striped background
/*  clr.alpha = 0xffff; clr.red = 0x2000;
  clr.green = 0x2000; clr.blue = 0x6000;
  XftColorAllocValue(dis, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0), &clr, &colr);
  for (int i = 20; i < 1200; i += 40)
          XftDrawRect(draw, &colr, i, 10, 20, 280);  */
            //Section: foreground  (alpha is enabled)
  clr.alpha = 0xffff; clr.red = 0xc000;
  clr.green = 0xc000; clr.blue = 0x8000;
  XftColorAllocValue(dis, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0), &clr, &colr);
  temp = XftNameParse("FreeSans:style=regular:pixelsize=32");
  FcResult result;
  patt = XftFontMatch(dis, DefaultScreen(dis), temp, &result);
  if (result == FcResultMatch) printf("pattern match is OK\n");
  XftNameUnparse (patt, dest, 272);
  printf(dest); printf("\n");
  font = XftFontOpenPattern(dis, patt);
  if (font) {
    printf("font is good\n");
    string fnt;
    fnt.assign(dest, dest + 59);
    XftDrawStringUtf8(draw, &colr, font, 20, 40,
            (unsigned char*)fnt.c_str(), fnt.size());
    string text = "The Quick Brown Fox Jumps Over The Lazy Dog";
    XftDrawStringUtf8(draw, &colr, font, 20, 76,
            (unsigned char*)text.c_str(), text.size());
  }
}

void microview() {
  int wy = 300, wx;
  for (int y = 0; y < 24; y++) {
    wx = 500;
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
  int top = (XDisplayHeight(dis, 0) - 700) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          1200, 700, 1, DefaultDepth(dis,0), InputOutput,
	  DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  draw = XftDrawCreate(dis, win, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0));
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 1200; sh.height = 700;
  sh.min_width = 600; sh.min_height = 350;
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

void close() {
  XUngrabPointer(dis, CurrentTime);
  XftFontClose(dis, font);
  XftDrawDestroy (draw);
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
}

int main() {
  init();
  KeySym key;
  char text;
  bool f[3];                 // f[0] event loop keeper,
  int mx, my;                // f[1] paint flag, f[2] pointergrabbed
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
        else printf("You pressed the %c key!\n", text);
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                f[0] = false;
        break;
    }
    if (f[1]) paint();
  }
  close();
}

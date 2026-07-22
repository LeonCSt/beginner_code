/*  leon005.cpp:  Window redraws when resized,
            using a shared memory buffer and XShmCreatePixmap(),
            to send window updates using XCopyArea().
    g++ leon005.cpp -o leon -lX11 -lXext                      [260514]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include <stdio.h>

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XShmSegmentInfo shminfo;
Pixmap pxmp;
int waW, waH, bufW, bufH; // working area width/height   and  buffer W/H
int cfgW, cfgH;           // notified configuration W/H
char *b;                  // bytes
unsigned *p;              // pixels
bool redraw = true;

void resize_draw() {
  redraw = false;
  bufW = cfgW; bufH = cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  unsigned d, c = 0xffaaaa66;
  int i, j, m, n, o;
  int k = (bufH / 2) * bufW + (bufW / 2);
  double r;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000033;
            //Section: Circle
  if (bufW <= bufH) r = bufW * 0.45;
  else r = bufH * 0.45;
  o = 0.7071 * r + 1.5;
  for (i = 0; i < o; i++) {
    j = (cos(asin(i / r)) * r + 0.5);
    n = i * bufW; m = j * bufW;
    p[k + i + m] = c; p[k + i - m] = c;
    p[k - i + m] = c; p[k - i - m] = c;
    p[k + j + n] = c; p[k + j - n] = c;
    p[k - j + n] = c; p[k - j - n] = c;
  }
            //Section: Rectangles
  XSetForeground(dis, gc, 0xffff7733);
  XDrawRectangle(dis, pxmp, gc, 2, 2, 100, 50);
  XDrawRectangle(dis, pxmp, gc, bufW - 102, bufH - 52, 100, 50);
}

void paint() {
  XCopyArea(dis, pxmp, win, gc, 0, 0, bufW, bufH, 0, 0);
  XFlush(dis);
  redraw = true;
}

void init() {
  dis = XOpenDisplay(0);
  waW = XDisplayWidth(dis, 0); waH = XDisplayHeight(dis, 0);
                  //Section: Set up shared memory
  shminfo.readOnly = False;
  shminfo.shmid = shmget(IPC_PRIVATE, waW * waH * 4,
          IPC_CREAT | 0777);
  shminfo.shmaddr = reinterpret_cast<char*>(shmat(shminfo.shmid, 0, 0));
  b = shminfo.shmaddr;
  p = reinterpret_cast<unsigned*>(shminfo.shmaddr);
  XShmAttach(dis, &shminfo);
                  //Section: Prep Window
  cfgW = 800; cfgH = 500;
  gc = XDefaultGC(dis, DefaultScreen(dis));
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = KeyReleaseMask | ButtonPressMask |
          PointerMotionMask | ExposureMask | StructureNotifyMask;
  int left = (waW - cfgW) / 2;
  int top = (waH - cfgH) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          cfgW, cfgH, 1, DefaultDepth(dis,0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PMaxSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = cfgW; sh.height = cfgH;
  sh.max_width = waW; sh.max_height = waH; 
  sh.min_width = 320; sh.min_height = 200;
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
  KeySym key;
  char text;
  bool loop = true, pf;
  int mx, my;
  loop = true;
  XEvent evnt;
  XConfigureEvent *xcfg;
  while(loop) {
    pf = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if ((key == XK_Escape) || (key == XK_q)) loop = false;
        break;
      case Expose:
        if (redraw && (cfgW != bufW || cfgH != bufH)) resize_draw();
        pf = true; break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loop = false;
        break;
    }
    if (pf) paint();
  }
  XShmDetach(dis, &shminfo);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);
  XDestroyWindow(dis, win);
}

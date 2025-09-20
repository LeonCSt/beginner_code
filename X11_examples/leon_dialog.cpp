/*  leon_dialog.cpp:  the pop-up dialog box
    maind() can be called from anywhere in leon_main.cpp .
    Intended to be modal on the parent, but focus can move
        to other clients.
    Execution resumes in the DestroyNotify event back in
        leon_main.cpp when this dialog closes.
    g++ leon_main.cpp leon_dialog.cpp -o leon -lX11      [250418]  */

#include "leon.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string>
using namespace std;

Window dlg;
int btnfocus;		// 0 none, 1 Off, 2 On, 3 Cancel
bool eventloop, paintflag, pointerdialog; 
string prmpt = "Would you like to toggle option 'X'?";

void response() {
  if (btnfocus == 1) {
    reply = false; paintflag = true;
  }
  else if (btnfocus == 2) {
    reply = true; paintflag = true;
  }
  if (btnfocus > 0) eventloop = false;
}

void bttns() {
  XDrawRectangle(dis, dlg, gc, 30, 68, 110, 50);
  XDrawRectangle(dis, dlg, gc, 169, 68, 110, 50);
  XDrawRectangle(dis, dlg, gc, 308, 68, 110, 50);
  XSetLineAttributes(dis, gc, 1, LineOnOffDash, CapButt, JoinMiter);
  if (reply) {
    XDrawRectangle(dis, dlg, gc, 159, 58, 130, 70);
  }
  else {
    XDrawRectangle(dis, dlg, gc, 20, 58, 130, 70);
  }
  XSetLineAttributes(dis, gc, 1, LineSolid, CapButt, JoinMiter);
  if (btnfocus > 0) {
    XSetForeground(dis, gc, 0xff332a33);
    if (btnfocus == 1) XFillRectangle(dis, dlg, gc, 33, 71, 104, 44);
    else if (btnfocus == 2)
            XFillRectangle(dis, dlg, gc, 172, 71, 104, 44);
    else XFillRectangle(dis, dlg, gc, 311, 71, 104, 44);
    XSetForeground(dis, gc, 0xff88aaaa);
  }
  string temp = "Off";
  XDrawString(dis, dlg, gc, 66, 103, temp.c_str(), 3);
  temp = "On";
  XDrawString(dis, dlg, gc, 207, 103, temp.c_str(), 2);
  temp = "Cancel";
  XDrawString(dis, dlg, gc, 324, 103, temp.c_str(), 6);
}

void paintd() {
  XClearWindow(dis, dlg);
  XDrawString(dis, dlg, gc, 10, 38, prmpt.c_str(), prmpt.size());
  bttns();
  XFlush(dis);
}

void initd() {
  int left, top, wdth, hght;		//dialog bounds
  left = mx - 200; top = my - 40; hght = 140;
  wdth = XTextWidth(font, prmpt.c_str(), prmpt.size()) + 20;
  if (wdth < 440) wdth = 440;
  if (left < 2) left = 2;
  else if (left > (796 - wdth)) left = (796 - wdth);
  if (top < 2) top = 2;
  else if (top > (596 - hght)) top = (596 - hght);
  dlg = XCreateSimpleWindow(dis, win, left, top,
          wdth, hght, 1, 0xff88aaaa, 0xff221922);
  XSelectInput(dis, dlg, StructureNotifyMask | PointerMotionMask |
          ButtonPressMask | EnterWindowMask | LeaveWindowMask |
          KeyReleaseMask | ExposureMask);
  XSetWMProtocols(dis, dlg, &WM_DELETE_WINDOW, 1);
  XSetForeground(dis, gc, 0xff88aaaa);
  XMapWindow(dis, dlg);
}

void maind() {
  initd();
  KeySym keyd;
  char textd;
  eventloop = true; pointerdialog = true;
  int i = 0; int j = 0;
  btnfocus = 0;
  XEvent evntd;
  while(eventloop) {
    paintflag = false;
    XNextEvent(dis, &evntd);
    switch(evntd.type) {
      case EnterNotify:
        pointerdialog = true;
        break;
      case LeaveNotify:
        pointerdialog = false;
        break;
      case Expose:
        paintflag = true;
        break;
      case MotionNotify:
        if (pointerdialog) {
          i = 0;
          mx = evntd.xbutton.x; my = evntd.xbutton.y;
          if ((my > 70) && (my < 115) && (mx > 32)) {
            j =((mx - 33) % 139);
            if  (j < 104) {
              i = ((mx - 33) / 139) + 1;
            }
          }
          if ((btnfocus == 0) && (i > 0) && (i < 4)) {
            btnfocus = i; paintflag = true;
          }
          else if ((btnfocus > 0) && (i < 4) && (btnfocus != i)) {
            btnfocus = i; paintflag = true;
          }
        }
        break;
      case ButtonPress:
        response();
        break;
      case KeyRelease:
        XLookupString(&evntd.xkey, &textd, 1, &keyd, 0);
        if (keyd == XK_Tab)  {
          btnfocus ++;
          if (btnfocus == 4) btnfocus = 0;
          paintflag = true;
        }
        else if (keyd == XK_Return) response();
        else if ((keyd == XK_Escape) || (keyd == XK_q))
                eventloop = false;
        break;
      case ClientMessage:
        if ((Atom) evntd.xclient.data.l[0] == WM_DELETE_WINDOW)
                eventloop = false;
        break;
    }
    if (paintflag) paintd();
  }
  XDestroyWindow(dis, dlg);
}


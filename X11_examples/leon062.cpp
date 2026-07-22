/*  leon062.cpp:  the pop-up dialog box
    maind() can be called from anywhere in the parent leon061.cpp .
    Intended to be modal on the parent, but focus can move
        to other clients.
    Execution resumes in the DestroyNotify event back in
        leon061.cpp when this dialog closes.
    g++ -I/usr/include/freetype2 leon061.cpp leon062.cpp -o leon -lX11 -lXext -lfreetype
                                                             [260524]  */

#include "leon060.h"

Window dlg;
Pixmap dxmp;
XRectangle drct, dbtns;
XRectangle btns[3];
int d_bfW, d_bfH, btnfocus;
bool eventloop, paintflag, d_redraw, pointerdialog;
unsigned *dp;
unsigned char ttl[] = "Would you like to toggle option 'X'?";
unsigned char lbls[] = "OFF\0ON\0\0Exit";

void response() {
  if (btnfocus == 0) reply = false;
  else if (btnfocus == 1) reply = true;
  eventloop = false;
}

void paintd() {
  XCopyArea(dis, dxmp, dlg, gc, drct.x, drct.y,
          drct.width, drct.height, drct.x, drct.y);
  XFlush(dis);
  d_redraw = true;}

void d_bttns(bool refresh) {
  d_redraw = false;
  int i, j;
  unsigned c, d;
  for (i = 0; i < 3; i++) {
    if (i == btnfocus) { c = 0xffaacccc; d = 0xff403040; }
    else { c = 0xff88aaaa; d = 0xff302030; }
    fill_rectangle(dp, d_bfW, d_bfH,
            btns[i].x, btns[i].y, btns[i].width, btns[i].height, d);
    j = text_width(&lbls[i * 4], 4);
    text_run(dp, d_bfW, d_bfH, btns[i].x + (btns[i].width - j) / 2,
        btns[i].y + txthght * 1.6, &lbls[i * 4], 4, c);
    if ((i == 0 && !reply) || (i == 1 && reply)) {
      fill_rectangle(dp, d_bfW, d_bfH, btns[i].x + txthght / 2,
              btns[i].y + txthght * 2.1, txthght * 2.5, txthght * 0.2, c);   
    }
  }
  if (refresh) { drct = dbtns; paintd(); }
}

void initd() {
  int left, top, margin, i, j;
  left = mX - txthght * 9; top = mY - txthght; d_bfH = txthght * 9;
  d_bfW = txthght * 18; margin = txthght / 6;
  if (left < margin) left = margin;
  else if (left > bufW - d_bfW) left = bufW - d_bfW - margin;
  if (top < margin) top = margin;
  else if (top > bufH - d_bfH) top = bufH - d_bfH - margin;
  for (i = 0; i < 3; i++) {
    btns[i].x = txthght * 2.3 + i * txthght * 5;
    btns[i].y = txthght * 4;
    btns[i].width = txthght * 3.5; btns[i].height = txthght * 2.7; 
  }
  dbtns.x = btns[0].x; dbtns.y = btns[0].y; dbtns.height = btns[0].height;
  dbtns.width = btns[0].width + btns[2].x - btns[0].x;
  dlg = XCreateSimpleWindow(dis, win, left, top,
          d_bfW, d_bfH, 1, 0xff88aaaa, 0xff221922);
  XSelectInput(dis, dlg, StructureNotifyMask | PointerMotionMask |
          ButtonPressMask | EnterWindowMask | LeaveWindowMask |
          KeyReleaseMask | ExposureMask);
  XSetWMProtocols(dis, dlg, &WM_DELETE_WINDOW, 1);
  dxmp = XShmCreatePixmap(dis, dlg, &shminfo.shmaddr[bufW * bufH * 4],
          &shminfo, d_bfW, d_bfH, 24);
  dp = &p[bufW * bufH];
  d_redraw = false;
  j = d_bfW * d_bfH;
  for (i = 0; i < j; i++) dp[i] = 0xff221922;
  i = strlen(reinterpret_cast<const char*>(ttl));
  text_run(dp, d_bfW, d_bfH, txthght * 2, txthght * 2,
          ttl, i, 0xff88aaaa);
  for (i = 0; i < 3; i++) draw_rectangle(dp, d_bfW, d_bfH, btns[i].x - 3,
          btns[i].y - 3, btns[i].width + 6, btns[i].height + 6, 0xff88aaaa);
  btnfocus = -1;
  d_bttns(false);
  XMapWindow(dis, dlg);
}

void maind() {
  initd();
  KeySym keyd;
  char textd;
  eventloop = true; pointerdialog = true;
  int i, j, k = btns[1].x - btns[0].x;
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
          i = -1;
          mX = evntd.xbutton.x; mY = evntd.xbutton.y;
          if (mY > dbtns.y && mY < dbtns.y + dbtns.height  && mX > dbtns.x) {
            j = (mX - dbtns.x) % k;
            if  (j < btns[0].width) i = (mX - dbtns.x) / k;
          }
          if (btnfocus == -1 && i > -1 && i < 3) {
            XDefineCursor(dis, win, hndcursor);
            btnfocus = i; d_bttns(true);
          }
          else if (btnfocus > -1 && i < 3 && btnfocus != i) {
            XDefineCursor(dis, win, None);
            btnfocus = i; d_bttns(true);
          }
        }
        break;
      case ButtonPress:
        if (btnfocus > -1) response();
        break;
      case KeyRelease:
        XLookupString(&evntd.xkey, &textd, 1, &keyd, 0);
        if (keyd == XK_Tab) {
          btnfocus ++;
          if (btnfocus == 3) btnfocus = -1;
          d_bttns(true);
        }
        else if (keyd == XK_Return && btnfocus > -1) response();
        else if (keyd == XK_Escape || keyd == XK_q) eventloop = false;
        break;
      case ClientMessage:
        if ((Atom) evntd.xclient.data.l[0] == WM_DELETE_WINDOW)
                eventloop = false;
        break;
    }
    if (paintflag) {
      drct.x = 0; drct.y = 0; drct.width = d_bfW; drct.height = d_bfH;
      paintd();
    }
  }
  XDestroyWindow(dis, dlg);
}

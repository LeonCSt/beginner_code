/*  leon070.cpp:   using the X11 clipboard
    select, cut and paste text - using the clipboard
    [mouse drag] select     [C] cut     [P] paste or [middlemouse btn]
    g++ leon070.cpp -o leon -lX11 -lXft -lfontconfig -I/usr/include/freetype2
                                                          [250707]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>
using namespace std;

Display *dis;
GC gc, tgc;
Window win, sel_owner_win;
Atom WM_DELETE_WINDOW, clpbrd, utf8str, prop;
Atom trgts, gdk_sel;
unsigned char *atom_ret = NULL;
unsigned long natoms;
XftFont *font;
XGlyphInfo extents;
Cursor txtcursor;
XftDraw *draw;
XRenderColor clr;
XftColor colr, hghcolr;
Pixmap txtbx;
vector<unsigned char> doc = {0};
            // ls[]   list of each lines start pos within doc[]
vector<unsigned short> ls = {0};
            // ll[]   list of pixel length of each line
vector<unsigned short> ll = {0};
int doclength = 0, caret = 0, nlines = 0, mx, my;
int caretx, carety, histcaret, histcaretx, histcarety;
            //   operation   0 is none,  1 is gettargets(),
	    //               2 is getclipboard()
int operation = 0;
bool loopkeep, paintflag, txtbxfocus, cursorcaret, txtbxactive;
bool blinker, blinkeractive, mousedown, highlight, select_own;
string selectn;

void gettargets() {
  Atom type, *targets;
  int di;
  unsigned long dul;
  char *an = NULL;
  XGetWindowProperty(dis, sel_owner_win, gdk_sel, 0,
          1024 * sizeof (Atom), False, XA_ATOM,
          &type, &di, &natoms, &dul, &atom_ret);
  printf("Targets:\n");
  targets = (Atom *)atom_ret;
  for (int i = 0; i < natoms; i++) {
    an = XGetAtomName(dis, targets[i]);
    printf("    '%s'\n", an);
  }
  if (an) XFree(an);
  XDeleteProperty(dis, sel_owner_win, gdk_sel);
}

void findcrsrpos() {
  int pos, num, tally = 0;
  char utf[4];
  char *dest = utf;
  string strng;
  if (my > 60 + nlines * 35) {
    caret = doclength;
    caretx = ll[nlines - 1] + 20;
    carety = (nlines - 1) * 35;
  }
  else {
    int j = (my - 60) / 35;
    carety = j * 35;
    if (mx - 80 > ll[j]) {
      caret = ls[j + 1];
      caretx = ll[j] + 20;
    }
    else if (mx < 80) {
      caret = ls[j];
      caretx = 20;
    }
    else {
      pos = ls[j];
      while (pos < ls[j + 1]) {
        caretx = tally;
        if (doc[pos] < 128) num = 1;
        else if (doc[pos] & 224 == 192) num = 2;
        else if (doc[pos] & 240 == 224) num = 3;
        else if (doc[pos] & 248 == 240) num = 4;
        else num = 1;
        if (doc[pos] == '\t') {
          strng = "\t";
          XftTextExtentsUtf8(dis, font, (unsigned char*)strng.c_str(),
                  num, &extents); tally += extents.xOff;
        }
        if (doc[pos] > 31) {
          for (int i = 0; i < num; i++) utf[i] = doc[pos + i];
          strng.assign(dest, dest + num);
          XftTextExtentsUtf8(dis, font, (unsigned char*)strng.c_str(),
                  num, &extents);
          tally += extents.xOff;
        }
        if (tally > (mx - 80)) break;
        pos += num;
      }
      caret = pos;
      caretx += 20;
    }
  }
}

void drawtxtbx() {
  if (txtbxfocus || txtbxactive) XSetForeground(dis, tgc, 0xff222255);
  else XSetForeground(dis, tgc, 0xff111144);
  XFillRectangle(dis, txtbx, tgc, 0, 0, 560, 420 );
  XSetForeground(dis, tgc, 0xffcccc88);
  if (doclength > 0) {
    int j = 27;
    string strng;
    for (int i = 0; i < nlines; i++) {
      if (i == 12) break;
      if (!highlight || ls[i + 1] <= histcaret || ls[i] >= caret) {
        strng.assign(doc.begin() + ls[i], doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &colr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if ((histcaret == ls[i]) && (caret == ls[i + 1])) {
        XFillRectangle(dis, txtbx, tgc, 20, j - 27, ll[i], 35);
        strng.assign(doc.begin() + ls[i], doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &hghcolr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if ((histcaret == ls[i]) && (caret < ls[i + 1])) {
        XFillRectangle(dis, txtbx, tgc, 20, carety, caretx - 20, 35);
        strng.assign(doc.begin() + ls[i], doc.begin() + caret);
        XftDrawStringUtf8(draw, &hghcolr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
        strng.assign(doc.begin() + caret, doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &colr, font, caretx, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if (histcaret > ls[i] && caret == ls[i + 1]) {
        strng.assign(doc.begin() + ls[i], doc.begin() + histcaret);
        XftDrawStringUtf8(draw, &colr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
        XFillRectangle(dis, txtbx, tgc, histcaretx, histcarety,
                ll[i] - histcaretx + 20, 35);
        strng.assign(doc.begin() + histcaret, doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &hghcolr, font, histcaretx, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if (histcaret < ls[i] && caret == ls[i + 1]) {
        XFillRectangle(dis, txtbx, tgc, 20, j - 27, ll[i], 35);
        strng.assign(doc.begin() + ls[i], doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &hghcolr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if (histcarety == carety) {
        strng.assign(doc.begin() + ls[i], doc.begin() + histcaret);
        XftDrawStringUtf8(draw, &colr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
        XFillRectangle(dis, txtbx, tgc, histcaretx, histcarety,
                caretx - histcaretx, 35);
        strng.assign(doc.begin() + histcaret, doc.begin() + caret);
        XftDrawStringUtf8(draw, &hghcolr, font, histcaretx, j,
                (unsigned char*)strng.c_str(), strng.size());
        strng.assign(doc.begin() + caret, doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &colr, font, caretx, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if (histcarety == j - 27) {
        strng.assign(doc.begin() + ls[i], doc.begin() + histcaret);
        XftDrawStringUtf8(draw, &colr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
        XFillRectangle(dis, txtbx, tgc, histcaretx, histcarety,
                ll[i] - histcaretx + 20, 35);
        strng.assign(doc.begin() + histcaret, doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &hghcolr, font, histcaretx, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else if (carety == j - 27) {
        XFillRectangle(dis, txtbx, tgc, 20, carety, caretx - 20, 35);
        strng.assign(doc.begin() + ls[i], doc.begin() + caret);
        XftDrawStringUtf8(draw, &hghcolr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
        strng.assign(doc.begin() + caret, doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &colr, font, caretx, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      else {
        XFillRectangle(dis, txtbx, tgc, 20, j - 27, ll[i], 35);
        strng.assign(doc.begin() + ls[i], doc.begin() + ls[i + 1]);
        if (doc[ls[i + 1] - 1] == '\n') strng.pop_back();
        XftDrawStringUtf8(draw, &hghcolr, font, 20, j,
                (unsigned char*)strng.c_str(), strng.size());
      }
      j += 35;
    }
  }
}

void castlines() {
  if (nlines != 0) {
    ls.erase(ls.begin(), ls.begin() + nlines);
    ll.erase(ll.begin(), ll.begin() + nlines);
    nlines = 0;
  }
  int pos = 0, num, tally = 0, space, spcpos, histpos = 0;
  char utf[4];
  char *dest = utf;
  string strng;
  ls[0] = doclength;
  while (pos < doclength) {
    if (pos == caret) {
      caretx = tally + 20;
      carety = nlines * 35;
    }
    if (doc[pos] < 128) num = 1;
    else if (doc[pos] & 224 == 192) num = 2;
    else if (doc[pos] & 240 == 224) num = 3;
    else if (doc[pos] & 248 == 240) num = 4;
    else num = 1;
    if (doc[pos] == '\n') {
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      histpos = pos + 1;
      tally = 0; space = 0;
      nlines ++;
      pos += num;
      continue;
    }
    if (doc[pos] == '\t') {
      strng = "\t";
      XftTextExtentsUtf8(dis, font, (unsigned char*)strng.c_str(),
              num, &extents);
      tally += extents.xOff;
      space = tally;
      spcpos = pos;
    }
    else if (doc[pos] > 31) {
      for (int i = 0; i < num; i++) utf[i] = doc[pos + i];
      strng.assign(dest, dest + num);
//      printf(strng.c_str()); printf("\n");
      XftTextExtentsUtf8(dis, font, (unsigned char*)strng.c_str(),
              num, &extents);
      tally += extents.xOff;
      if (doc[pos] == 32) {
         space = tally;
         spcpos = pos;
      }
    }
    if (tally > 520) {
      ls.insert(ls.begin() + nlines, histpos);
      if (space == 0) {
        ll.insert(ll.begin() + nlines, tally);
        histpos = pos + num;
      }
      else {
        ll.insert(ll.begin() + nlines, space);
        histpos = spcpos + 1;
        pos = spcpos;
      }
      tally = 0; space = 0;
      nlines ++;
    }
    else if (pos == doclength - num) {
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      nlines ++;
    }
    pos += num;
  }
//  printf("number of lines = %d\n", nlines);
}

void sendtoclipboard() {
  if (!select_own)
          XSetSelectionOwner(dis, clpbrd, sel_owner_win, CurrentTime);
  if (XGetSelectionOwner(dis, clpbrd) == sel_owner_win) {
    select_own = true;
    selectn.assign(doc.begin() + histcaret, doc.begin() + caret);
//    printf(selectn.c_str()); printf("\n");
    int j = caret - histcaret;
    for (int i =  caret; i <= doclength; i++) doc[i - j] = doc[i];
    doclength -= j;
    caret -= j;
    doc.resize(doclength + 1);
    castlines();
    if (nlines > 12) {
      caret = ls[12];
      carety = 385;
      caretx = ll[11] + 20;
    }
    if (doclength == 0) {
      blinkeractive = false;
      txtbxactive = false;
    }
    highlight = false;
    drawtxtbx();
    paintflag = true;
  }
}

void getclipboard() {
  unsigned char *bytes;
  int actual_format;
  unsigned long nitems, bytes_after;
  if (select_own) nitems = selectn.size();
  else XGetWindowProperty(dis, win, prop, 0, LONG_MAX/4, False,
           AnyPropertyType, &utf8str, &actual_format,
           &nitems, &bytes_after, &bytes);
  if ((nitems + doclength) > 65534) {
    printf("document plus clipboard contents too large\n");
    if (!select_own) {
      XFree(bytes);
      XDeleteProperty(dis, win, prop);
    }
  }
  else {
    doc.resize(doclength + nitems + 1);
    for (int i = doclength; i >= caret; i--) doc[i + nitems] = doc[i];
    int j = caret;
    if (select_own) {
      for (int i = 0; i < nitems; i++) {
        doc[j] = selectn[i];
        j++;
      }
    }
    else {
      for (int i = 0; i < nitems; i++) {
        doc[j] = bytes[i];
        j++;
      }
    }
    caret += nitems;
    doclength += nitems;
//    printf("%.*s", doclength, doc.begin()); printf("\n");
    if (!select_own) {
      XFree(bytes);
      XDeleteProperty(dis, win, prop);
    }
    castlines();
    if (nlines > 12) {
      caret = ls[12];
      carety = 385;
      caretx = ll[11] + 20;
    }
    drawtxtbx();
    paintflag = true;
  }
}

void paint() {
  XClearWindow(dis, win);
  XCopyArea(dis, txtbx, win, gc, 0, 0, 560, 420, 60, 60);
  XFlush(dis);
}

void caretblinker() {
  blinker = true;
  int i = 0;
  while (blinkeractive) {
    if (i % 20 == 0) {
      if (blinker) XDrawLine(dis, txtbx, tgc, caretx, carety,
              caretx, carety + 34);
      else drawtxtbx();
      paint();
      blinker = !blinker;
    }
    this_thread::sleep_for(chrono::milliseconds(23));
    i++;
  }
  drawtxtbx(); paint();
}

void init() {
  dis = XOpenDisplay(0);
                //Section: Prep main window
  int left = (XDisplayWidth(dis, 0) - 800) / 2;
  int top = (XDisplayHeight(dis, 0) - 600) / 2;
  gc = XDefaultGC(dis, DefaultScreen(dis));
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = StructureNotifyMask | PointerMotionMask |
          ButtonPressMask | ButtonReleaseMask |
          KeyReleaseMask | ExposureMask;
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          800, 600, 1, DefaultDepth(dis,0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts; XTextProperty nm, icnm;
  sh.flags = PPosition | PSize | PMinSize | PMaxSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = 800; sh.height = 600;
  sh.min_width = 800; sh.min_height = 600;
  sh.max_width = 800; sh.max_height = 600;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  char snm[] = "Test GUI(Title Bar Text)";
  char *psnm = snm;
  XStringListToTextProperty(&psnm, 1, &nm);
  char sicnm[] = "Test GUI(Icon Text)";
  char *psicnm = sicnm;
  XStringListToTextProperty(&psicnm, 1, &icnm);
  XSetWMProperties(dis, win, &nm, &icnm, NULL, 0, &sh, &hnts, NULL);
                //Section: Prep textbox
  clr.alpha = 0xffff; clr.red = 0xbb00;
  clr.green = 0xbb00; clr.blue = 0x7700;
  XftColorAllocValue(dis, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0), &clr, &colr);
  clr.alpha = 0xffff; clr.red = 0x1100;
  clr.green = 0x1100; clr.blue = 0x4400;
  XftColorAllocValue(dis, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0), &clr, &hghcolr);
  txtbx = XCreatePixmap(dis, win, 560, 420, 24);
  XGCValues tgcv;
  tgcv.function = GXcopy; tgcv.foreground = 0xffaaaa66;
  tgcv.background = 0xff000033;
  tgc = XCreateGC(dis, txtbx, GCForeground | GCBackground, &tgcv);
  draw = XftDrawCreate(dis, txtbx, DefaultVisual(dis, 0),
          DefaultColormap(dis, 0));
  drawtxtbx();
  txtcursor = XCreateFontCursor(dis, XC_xterm);
                //Section: Prep font
  FcResult result;
  FcPattern *trypatt, *patt;
  loopkeep = true;
  trypatt = XftNameParse("FreeSerif:style=regular:pixelsize=32");
  patt = XftFontMatch(dis, DefaultScreen(dis), trypatt, &result);
  if (result != FcResultMatch) {
    loopkeep = false;
    printf("failed to match a font\n");
  }
  else {
    font = XftFontOpenPattern(dis, patt);
    if (!font) {
      loopkeep = false;
      printf("font failed to load\n");
    }
  }
                //Section: Prep selections
  sel_owner_win = XCreateSimpleWindow(dis, XDefaultRootWindow(dis),
          -10, -10, 1, 1, 0, 0, 0);
  clpbrd = XInternAtom(dis, "CLIPBOARD", False);
  utf8str = XInternAtom(dis, "UTF8_STRING", False);
  trgts = XInternAtom(dis, "TARGETS", False);
  prop = XInternAtom(dis, "XSEL_DATA", False);
  gdk_sel = XInternAtom(dis, "GDK_SELECTION", False);
                //
  XMapWindow(dis, win);
}

void shutdown() {
  XFree(atom_ret);
  XDestroyWindow(dis, sel_owner_win);
  XFreeCursor(dis, txtcursor);
  XftFontClose(dis, font);
  XftDrawDestroy(draw);
  XFreeGC(dis, tgc);
  XDestroyWindow(dis, win);
}

int main() {
  init();
  char text;
  int endline, enddoc;
  XEvent evnt;
  KeySym key;
  XSelectionRequestEvent *sev;
  XSelectionEvent ssev;
  while(loopkeep) {
    paintflag = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case Expose:
        paintflag = true; break;
      case MotionNotify:
        mx = evnt.xbutton.x; my = evnt.xbutton.y;
                    //section:  textbox focus
        if (mx > 60 && mx < 620 && my > 60 && my < 480
                && !txtbxfocus) {
          txtbxfocus = true;
          if (!txtbxactive) {
            drawtxtbx(); paintflag = true;
          }
        }
        else if ((mx < 61 || mx > 619 || my < 61 || my > 479)
                && txtbxfocus) {
          txtbxfocus = false;
          if (!txtbxactive) {
            drawtxtbx(); paintflag = true;
          }
        }
                    //section:  mouse over text
        enddoc = 60 + 35 * nlines;
        if (enddoc > 480) enddoc = 480;
        if (my < 60 || my > enddoc || nlines == 0) endline = 80;
        else endline = 80 + ll[(my - 60) / 35];
        if (my > 60 && my < enddoc && mx > 80
                && mx < endline && !cursorcaret) {
          cursorcaret = true;
          XDefineCursor(dis, win, txtcursor);
        }
        else if ((my < 60 || my > enddoc || mx < 80
                || mx > endline) && cursorcaret) {
          cursorcaret = false;
          XDefineCursor(dis, win, None);
        }
                    //section: mouse drag to highlight text
        if (mousedown && my < enddoc) {
          findcrsrpos();
          if (caret > histcaret) {
            highlight = true;
            drawtxtbx();
            paintflag = true;
          }
        }
        break;
      case ButtonPress:
        if (evnt.xbutton.button == 1) {
          if (txtbxfocus) {
            if (doclength == 0) break;
            findcrsrpos();
            histcaret = caret;
            histcaretx = caretx; histcarety = carety;
            mousedown = true;
            blinkeractive = false;
            txtbxactive = true;
          }
          else {
            txtbxactive = false;
            blinkeractive = false;
            drawtxtbx();
            paintflag = true;
          }
        }
        break;
      case ButtonRelease:
        if (evnt.xbutton.button == 1 && txtbxactive) {
          mousedown = false;
          if (caret > histcaret) highlight = true;
          else highlight = false;
          blinkeractive = true;
          thread t1(caretblinker); t1.detach();
        }
        else if (evnt.xbutton.button == 2 && txtbxfocus) {
          if (select_own) getclipboard();
          else {
            operation = 2;
            XConvertSelection(dis, clpbrd, utf8str, prop, win,
                    CurrentTime);
          }
        }
        break;
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Escape || key == XK_q) loopkeep = false;
        else if (key == XK_p && txtbxfocus) {
          if (select_own) getclipboard();
          else {
            operation = 2;
            XConvertSelection(dis, clpbrd, utf8str, prop, win,
                    CurrentTime);
          }
        }
        else if (key == XK_c && highlight) {
          operation = 1;
          XConvertSelection(dis, clpbrd, trgts, gdk_sel,
                   sel_owner_win, CurrentTime);
        }
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loopkeep = false;
        break;
      case SelectionClear:
        printf("lost ownership\n");
        select_own = false;
        selectn = "";
        break;
      case SelectionNotify:
        if (evnt.xselection.selection == clpbrd
                && evnt.xselection.property) {
          if (operation == 1) {
            gettargets();
            sendtoclipboard();
          }
          else if (operation == 2) getclipboard();
          operation = 0;
        }
        break;
      case SelectionRequest:
        sev = (XSelectionRequestEvent*)&evnt.xselectionrequest;
        printf("Requestor: 0x%lx\n", sev->requestor);
        char *an;
        if (sev->property == None) {
          an = XGetAtomName(dis, sev->target);
          printf("Denying request of type '%s'\n", an);
        }
        else if (sev->target == trgts) {
          an = XGetAtomName(dis, sev->property);
          printf("Sending Atoms to window 0x%lx, property '%s'\n",
                  sev->requestor, an);
          XChangeProperty(dis, sev->requestor, sev->property,
                  XA_ATOM, 32, PropModeReplace, atom_ret, natoms);
        }
        else {
          an = XGetAtomName(dis, sev->property);
          printf("Sending data to window 0x%lx, property '%s'\n",
                  sev->requestor, an);
          XChangeProperty(dis, sev->requestor, sev->property,
                  utf8str, 8,  PropModeReplace,
                  (unsigned char *)selectn.c_str(), selectn.size());
        }
        if (an) XFree(an);
        ssev.property = sev->property;
        ssev.target = sev->target;
        ssev.type = SelectionNotify;
        ssev.requestor = sev->requestor;
        ssev.selection = sev->selection;
        ssev.time = sev->time;
        XSendEvent(dis, sev->requestor, True, NoEventMask,
                (XEvent *)&ssev);
        break;
    }
    if (paintflag) paint();
  }
  shutdown();
}

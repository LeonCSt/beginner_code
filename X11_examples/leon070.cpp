/*  leon070.cpp:   using the X11 clipboard
    select, cut and paste text - using the clipboard
    [mouse drag]  select      [LeftCtrl][C]  cut      [LeftCtrl][V]  paste
    Textbox starts out empty so you have to copy
    from somewhere else then paste in to get started.
    example: copy this --> "Utf8 encoding L♡VE, j☺y, pe☮ce. Regards ←"
        (In gvim it is [Shift] ['][=]  then  [Y] to copy to clipboard.)
    Use mouse to highlight or place caret in text.
        (Not implemented: Keyboard keys to highlight or move caret.)
    [Q] or [ESC] to close

    g++ -I/usr/include/freetype2 leon070.cpp -o leon -lX11 -lXext -lfreetype                                                     [260527]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <stdio.h>
#include <thread>
#include <chrono>
using namespace std;

Display *dis;
GC gc;
Window win, sel_owner_win;
Atom WM_DELETE_WINDOW, clpbrd, utf8str, prop;
Atom trgts, gdk_sel;
unsigned char *atom_ret = NULL;
unsigned long natoms;
XShmSegmentInfo shminfo;
FT_Library lbrry;
FT_Face face;
Cursor txtcursor;
Pixmap pxmp;
unsigned *p; //pixels
XRectangle rct, tbrct, mgrct;
vector<unsigned char> doc = {0};
vector<unsigned char> clip = {0};
vector<unsigned> ls = {0}; // ls[]  list of each lines start pos within doc[]
vector<unsigned short> ll = {0}; // ll[]  list of pixel length of each line
atomic<int> caret;
int waW, waH, cfgW, cfgH, bufW, bufH, txthght, tabW; 
int doclength, nlines, enddoc, mX, mY, lineH, ln_o;
int caretx, carety, histcaret, histcaretx, histcarety;
int operation; // 0 is none,  1 is gettargets(), 2 is getclipboard()
atomic<bool> blinkeractive, redraw = true;
bool loop, pf, txtbxfocus, lftctl, cursorcaret, txtbxactive;
bool blinker, mousedown, highlight, select_own;
unsigned char gct[256]; // gamma correction table
unsigned c[] = {0xff000033, 0xff111144, 0xff222255, 0xffaaaa66, 0xffcccc88};


void draw_rectangle(unsigned *m, int bfW, int bfH, int X, int Y,
        int width, int height, unsigned fg) {
  if (X < 0 || X > bfW - width
            || Y < 0 || Y > bfH - height) return;
  int g, h, i, k;
  k = Y * bfW + X; h = (Y + height - 1) * bfW + X;
  for (i = 0; i < width; i++) {
    m[k] = fg; m[h] = fg;
    k += 1; h += 1;
  }
  g = height - 2; k = (Y + 1) * bfW + X;
  h = (Y + 1) * bfW + X + width - 1;
  for (i = 0; i < g; i++) {
    m[k] = fg; m[h] = fg;
    k += bfW; h += bfW;
  }
}

void fill_rectangle(unsigned *m, int bfW, int bfH,
        int X, int Y, int width, int height, unsigned fg) {
  if (X < 0 || X > bfW - width
            || Y < 0 || Y > bfH - height) return;
  int i, j, k, n;
  if ((fg & 0xff000000) == 0xff000000) {
    n = Y * bfW + X;
    j = Y + height; k = X + width;
    for (Y; Y < j; Y++) {
      for (i = X; i < k; i++) {
       m[n] = fg;
       n++;
      }
    n += bfW - width;
    }
  }
  else {
    // implement transparancy
  }
}

void draw_glyph(unsigned *m, int bfW, int bfH, int X, int Y,
        unsigned fg, unsigned bg) {
  if (Y < 0 || Y > bfH - face->glyph->bitmap.rows
          || X < 0 || X > bfW - face->glyph->bitmap.width) return;
  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return;
  unsigned char fR, fG, fB, bR, bG, bB;
  unsigned char *g = &face->glyph->bitmap.buffer[0];
  unsigned d;
  int i, j, k = 0, n, o;
  double r;
  fR = ((fg & 0xff0000) >> 16); fG = ((fg & 0xff00) >> 8);
  fB = (fg & 0xff); bR = ((bg & 0xff0000) >> 16);
  bG = ((bg & 0xff00) >> 8); bB = (bg & 0xff);
  j = Y + face->glyph->bitmap.rows;
  i = X + face->glyph->bitmap.width;
  for (Y; Y < j; Y++) {
    for (n = X; n < i; n++) {
      if (g[k] == 0) {}
      else if (g[k] == 255) {
        m[Y * bfW + n] = fg;
      }
      else {
        d = 0xff000000;
        r = gct[g[k]] / 255.0;
        o = r * fR + (1 - r) * bR + 0.5;
        d += o << 16;
        o = r * fG + (1 - r) * bG + 0.5;
        d += o << 8;
        o = r * fB + (1 - r) * bB + 0.5;
        d += o;
        m[Y * bfW + n] = d;
      }
      k++;
    }
  }
}

void text_run(unsigned *m, int bfW, int bfH, int X, int Y,
        unsigned char *str, int length, unsigned fg) {
  int pos = 0, num, i, j, k;
  unsigned u, gi, bg;
  i = txthght * face->ascender / 1152 + 1;
  j = txthght * face->descender / -1152 + 1;
  k = txthght * 0.63;
  if (X > bfW - k || X < 0 || Y > bfH - j || Y < i) return;
                    //Section: start run
  while (pos < length) {
    if (str[pos] < 128) {
      num = 1;
      u = str[pos];
    }
    else if ((str[pos] & 224) == 192) {
      num = 2;
      u = ((str[pos] & 31) << 6) + (str[pos + 1] & 63);
    }
    else if ((str[pos] & 240) == 224) {
      num = 3;
      u = ((str[pos] & 15) << 12) + ((str[pos + 1] & 63) << 6)
              + (str[pos + 2] & 63);
    }
    else if ((str[pos] & 248) == 240) {
      num = 4;
      u = ((str[pos] & 7) << 18) + ((str[pos + 1] & 63) << 12)
              + ((str[pos + 2] & 63) << 6) + (str[pos + 3] & 63);
    }
    else {
      pos++; continue;
    }
    if (u == 0) break; 
    else if (u == 10) {
      X = 0; Y += txthght + 5;
      if (Y > bfH - j) break;
    }
    else if(u == 9) {
      X += tabW - (X - tbrct.x) % tabW;
      if (X > bfW - k) {
        X = 0; Y += txthght * 1.2;
        if (Y > bfH - j) break;
      }
    }
    else if (u < 32 || (u > 127 && u < 161)) {
      pos += num; continue;
    }
    else {
      gi = FT_Get_Char_Index(face, u);
      if (FT_Load_Glyph(face, gi, 0)) break;
      if (face->glyph->bitmap.width != 0) {
        bg = m[(Y - (txthght / 3)) * bfW +
                X + ((face->glyph->advance.x >> 6) / 2)];
        draw_glyph(m, bfW, bfH, face->glyph->bitmap_left + X,
                Y - face->glyph->bitmap_top, fg, bg);
      }
      X += face->glyph->advance.x >> 6;
      if (X > bfW - k) {
        X = 0; Y += txthght * 1.2;
        if (Y > bfH - j) break;
      }
    }
    pos += num;
  }
}

void gettargets() {
  Atom type, *targets;
  int di;
  unsigned long dul;
  XGetWindowProperty(dis, sel_owner_win, gdk_sel, 0,
          1024 * sizeof (Atom), False, XA_ATOM,
          &type, &di, &natoms, &dul, &atom_ret);
  printf("You are offering. Target these types: -\n");
  targets = (Atom *)atom_ret;
  char *name;
  for (int i = 0; i < natoms; i++) {
    name = XGetAtomName(dis, targets[i]);
    printf("    '%s'\n", name);
    XFree(name);   
  }
  XDeleteProperty(dis, sel_owner_win, gdk_sel);
}

void findcrsrpos() {
  int pos, num, tally = tbrct.x, j;
  unsigned u, gi;
  if (mY > tbrct.y + nlines * lineH) {
    caret = doclength;
    caretx = tbrct.x + ll[nlines - 1];
    carety = tbrct.y + (nlines - 1) * lineH;
  }
  else {
    j = (mY - tbrct.y) / lineH;
    carety = tbrct.y + j * lineH;
    if (mX - tbrct.x > ll[j]) {
      caret = ls[j + 1];
      caretx = tbrct.x + ll[j];
    }
    else if (mX < tbrct.x) {
      caret = ls[j];
      caretx = tbrct.x;
    }
    else {
      pos = ls[j];
      while (pos < ls[j + 1]) {
        caretx = tally;
        if (doc[pos] < 128) {
          num = 1;
          u = doc[pos];
        }
        else if ((doc[pos] & 224) == 192) {
          num = 2;
          u = ((doc[pos] & 31) << 6) + (doc[pos + 1] & 63);
        }
        else if ((doc[pos] & 240) == 224) {
          num = 3;
          u = ((doc[pos] & 15) << 12) + ((doc[pos + 1] & 63) << 6)
                  + (doc[pos + 2] & 63);
        }
        else if ((doc[pos] & 248) == 240) {
          num = 4;
          u = ((doc[pos] & 7) << 18) + ((doc[pos + 1] & 63) << 12)
                  + ((doc[pos + 2] & 63) << 6) + (doc[pos + 3] & 63);
        }
        else {
          pos++; continue;
        }
        if (doc[pos] == 9) tally += tabW - (tally - tbrct.x) % tabW;
        else if (u < 32 || (u > 127 && u < 161)) {
          pos += num; continue;
        }
        else {
          gi = FT_Get_Char_Index(face, u);
          if (FT_Load_Glyph(face, gi, 0)) { pos += num; continue; }
          tally += face->glyph->advance.x >> 6;
        }
        if (tally > mX) break;
        pos += num;
      }
      caret = pos;
    }
  }
}

void paint() {
  XCopyArea(dis, pxmp, win, gc, rct.x, rct.y,
          rct.width, rct.height, rct.x, rct.y);
  XFlush(dis);
  redraw = true;
}

void drawtxtbx(bool refresh) {
  redraw = false;
  int i, j, k;
  unsigned bg, fg;
  if (txtbxfocus || txtbxactive) { bg = c[2]; fg = c[4]; }
  else { bg = c[1]; fg = c[3]; }
  fill_rectangle(p, bufW, bufH, mgrct.x, mgrct.y,
          mgrct.width, mgrct.height, bg);
  if (doclength > 0) {
    j = tbrct.y + ln_o;
    for (i = 0; i < nlines; i++) {
      if (i == 12) break;
      if (!highlight || ls[i + 1] <= histcaret || ls[i] >= caret) {
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(p, bufW, bufH, tbrct.x, j, &doc[ls[i]], k, fg);
      }
      else if ((histcaret <= ls[i]) && (caret >= ls[i + 1])) {
        fill_rectangle(p, bufW, bufH, tbrct.x, j - ln_o,
                ll[i], lineH, fg);
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(p, bufW, bufH, tbrct.x, j, &doc[ls[i]], k, bg);
      }
      else if ((histcaret <= ls[i]) && (caret < ls[i + 1])) {
        fill_rectangle(p, bufW, bufH, tbrct.x, j - ln_o,
                caretx - tbrct.x, lineH, fg);
        k = caret - ls[i];
        text_run(p, bufW, bufH, tbrct.x, j, &doc[ls[i]], k, bg);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(p, bufW, bufH, caretx, j, &doc[caret],
                k, fg);
      }
      else if (histcaret > ls[i] && caret >= ls[i + 1]) {
        k = histcaret - ls[i];
        text_run(p, bufW, bufH, tbrct.x, j, &doc[ls[i]], k, fg);
        fill_rectangle(p, bufW, bufH, histcaretx, histcarety,
                ll[i] + tbrct.x - histcaretx, lineH, fg);
        k = ls[i + 1] - histcaret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(p, bufW, bufH, histcaretx, j, &doc[histcaret], k, bg);
      }
      else {
        k = histcaret - ls[i];
        text_run(p, bufW, bufH, tbrct.x, j, &doc[ls[i]], k, fg);
        fill_rectangle(p, bufW, bufH, histcaretx, histcarety,
                caretx - histcaretx, lineH, fg);
        k = caret - histcaret;
        text_run(p, bufW, bufH, histcaretx, j, &doc[histcaret], k, bg);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(p, bufW, bufH, caretx, j, &doc[caret], k, fg);
      }
      j += lineH;
    }
  }
  if (refresh) { rct = mgrct; paint(); }
}

void castlines() {
  if (nlines != 0) {
    ls.erase(ls.begin(), ls.begin() + nlines);
    ll.erase(ll.begin(), ll.begin() + nlines);
    nlines = 0;
  }
  int pos = 0, num, tally = 0, space, spcpos, histpos = 0;
  unsigned u, gi;
  ls[0] = doclength;
  while (pos < doclength) {
    if (pos == caret) {
      caretx = tbrct.x + tally;
      carety = tbrct.y + nlines * lineH;
    }
    if (doc[pos] < 128) {
      num = 1;
      u = doc[pos];
    }
    else if ((doc[pos] & 224) == 192) {
      num = 2;
      u = ((doc[pos] & 31) << 6) + (doc[pos + 1] & 63);
    }
    else if ((doc[pos] & 240) == 224) {
      num = 3;
      u = ((doc[pos] & 15) << 12) + ((doc[pos + 1] & 63) << 6)
              + (doc[pos + 2] & 63);
    }
    else if ((doc[pos] & 248) == 240) {
      num = 4;
      u = ((doc[pos] & 7) << 18) + ((doc[pos + 1] & 63) << 12)
              + ((doc[pos + 2] & 63) << 6) + (doc[pos + 3] & 63);
    }
    else {
      pos++; continue;
    }
    if (u == 10) {
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      histpos = pos + 1;
      tally = 0; space = 0;
      nlines ++;
      pos ++;
      continue;
    }
    if (u == 9) {
      tally += tabW - tally % tabW;
      space = tally;
      spcpos = pos;
    }
    else if (u < 32 || (u > 127 && u < 161)) {
      pos += num; continue;
    }
    else {
      gi = FT_Get_Char_Index(face, u);
      if (FT_Load_Glyph(face, gi, 0)) break;
      tally += face->glyph->advance.x >> 6;
      if (u == 32) {
        space = tally;
        spcpos = pos;
      }
    }
    if (tally > 24 * txthght) {
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
      if (caret == doclength) {
        caretx = tbrct.x + tally;
        carety = tbrct.y + nlines * lineH;
      }
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      nlines ++;
    }
    pos += num;
  }
  //printf("number of lines = %d\n", nlines);
  if (nlines > 12) {
    caret = ls[12];
    carety = tbrct.y + 11 * lineH;
    caretx = tbrct.x + ll[11];
    enddoc = tbrct.y + tbrct.height;
  }
  else enddoc = tbrct.y + nlines * lineH;
  if (redraw) drawtxtbx(true);
}

void sendtoclipboard() {
  int i, j, k = 0;
  if (!select_own)
          XSetSelectionOwner(dis, clpbrd, sel_owner_win, CurrentTime);
  if (XGetSelectionOwner(dis, clpbrd) == sel_owner_win) {
    select_own = true;
    j = caret - histcaret;
    clip.resize(j + 1);
    for (i = histcaret; i < caret; i++) { clip[k] = doc[i]; k++; }
    clip[k] = 0;
    //printf("%.*s", j, &clip[0]); printf("\n");
    for (i = caret; i <= doclength; i++) doc[i - j] = doc[i];
    doclength -= j;
    caret -= j;
    doc.resize(doclength + 1);
    castlines();
    if (doclength == 0) {
      blinkeractive = false;
      txtbxactive = false;
    }
    highlight = false;
    if (redraw) drawtxtbx(true);
  }
}

void getclipboard() {
  unsigned char *bytes;
  int actual_format;
  unsigned long nitems, bytes_after;
  if (select_own) nitems = clip.size() - 1;
  else {
    printf("piping in\n");
    XGetWindowProperty(dis, win, prop, 0, LONG_MAX/4, False,
           AnyPropertyType, &utf8str, &actual_format,
           &nitems, &bytes_after, &bytes);
  }
  if ((nitems + doclength) > 200000) {
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
        doc[j] = clip[i];
        j++;
      }
    }
    else {
      for (int i = 0; i < nitems; i++) {
        doc[j] = bytes[i];
        j++;
      }
      XFree(bytes);
      XDeleteProperty(dis, win, prop);
    }
    caret += nitems;
    doclength += nitems;
    //printf("%.*s", nitems, &doc[caret - nitems]); printf("\n");
    castlines();
  }
}

void caretblinker() {
  blinker = true;
  int i = 0, j, k, x, y;
  unsigned h[lineH];
  this_thread::sleep_for(chrono::milliseconds(23));
  while (blinkeractive) {
    if (i % 20 == 0 && redraw) {
      redraw = false;
      x = caretx; y = carety; j = y * bufW + x;
      if (blinker) {
        for (k = 0; k < lineH; k++) {
          h[k] = p[j];
          p[j] = 0xffaaaa66;
          j += bufW;	  
        }
      }
      else {
        for (k = 0; k < lineH; k++) {
          p[j] = h[k];
          j += bufW;	  
        }
      }
      rct.x = x; rct.y = y; rct.width = 1; rct.height = lineH;
      thread t2(paint); t2.detach();
      blinker = !blinker;
    }
    this_thread::sleep_for(chrono::milliseconds(23));
    i++;
  }
  if (redraw) drawtxtbx(true);
}

void resize_draw() {
  redraw = false;
  bufW = cfgW; bufH =cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  int i, j, k;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000033;
  k = txthght * 0.62;
  draw_rectangle(p, bufW, bufH, mgrct.x - 3, mgrct.y - 3,
          mgrct.width + 6, mgrct.height + 6, c[3]);
  drawtxtbx(false);
}

void init() {
  dis = XOpenDisplay(0);
  waW = XDisplayWidth(dis, 0); waH = XDisplayHeight(dis, 0);
  txthght = 36; // <-- you can alter      range  10 -> 50
  if (txthght < 10 || txthght > 50) { printf("txthght: out of bounds\n");
          loop = false; return; }
  cfgW = txthght * 31; cfgH = txthght * 19;
  gc = XDefaultGC(dis, DefaultScreen(dis));
                //Section: Prep main window
  int attriMask = CWBackPixel | CWWinGravity | CWEventMask;
  XSetWindowAttributes winAttr;
  winAttr.background_pixel = 0xff000033;
  winAttr.win_gravity = CenterGravity;
  winAttr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
          ButtonReleaseMask | PointerMotionMask | ExposureMask |
          StructureNotifyMask;
  int left = (waW - cfgW) / 2;
  int top = (waH - cfgH) / 2;
  win = XCreateWindow(dis, XDefaultRootWindow(dis), left, top,
          cfgW, cfgH, 1, DefaultDepth(dis,0), InputOutput,
          DefaultVisual(dis, 0), attriMask, &winAttr);
  WM_DELETE_WINDOW = XInternAtom(dis, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dis, win, &WM_DELETE_WINDOW, 1);
  XSizeHints sh; XWMHints hnts;
  sh.flags = PPosition | PSize | PMinSize | PMaxSize | PWinGravity;
  sh.x = left; sh.y = top; sh.width = cfgW; sh.height = cfgH;
  sh.max_width = waW; sh.max_height = waH; 
  sh.min_width = txthght * 28; sh.min_height = txthght * 18;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  XStoreName(dis, win, "Test GUI(Title Bar Text)");
  XSetIconName(dis, win, "Test GUI(Icon Text)");
  XSetWMNormalHints(dis, win, &sh);
  XSetWMHints(dis, win, &hnts);
                  //Section: Set up shared memory
  shminfo.readOnly = False;
  shminfo.shmid = shmget(IPC_PRIVATE, waW * waH * 4 + txthght * txthght * 648,
          IPC_CREAT | 0777); 
  shminfo.shmaddr = reinterpret_cast<char*>(shmat(shminfo.shmid, 0, 0));
  p = reinterpret_cast<unsigned*>(shminfo.shmaddr);
  XShmAttach(dis, &shminfo);
                  //Section: Prep font
  FT_Init_FreeType(&lbrry);
  if (FT_New_Face(lbrry, "/usr/share/fonts/gnu-free/FreeSerif.otf",
          0, &face)) {printf("font not loaded\n"); loop = false; return;}
  //printf("number of glyphs in this font = %d\n", face->num_glyphs);
  FT_Set_Pixel_Sizes(face, 0, txthght);
  txtcursor = XCreateFontCursor(dis, XC_xterm);
                  //Section: Set up gamma correction table
  double q, d;
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
  int j = 0;
  d = 255.5;
  for (int i = 255; i > -1; i--) {
    gct[i] = d;
    if (i % 16 == 15) {
       q = cv[j] / 16.0;
       j++;
    }
    d -= q;
  }
                //Section: Prep selections
  sel_owner_win = XCreateSimpleWindow(dis, XDefaultRootWindow(dis),
          -10, -10, 1, 1, 0, 0, 0);
  clpbrd = XInternAtom(dis, "CLIPBOARD", False);
  utf8str = XInternAtom(dis, "UTF8_STRING", False);
  trgts = XInternAtom(dis, "TARGETS", False);
  prop = XInternAtom(dis, "XSEL_DATA", False);
  gdk_sel = XInternAtom(dis, "GDK_SELECTION", False);
                //Section: Set up sizes
  FT_Load_Glyph(face, FT_Get_Char_Index(face, 48), 0);
  tabW = 8 * face->glyph->advance.x >> 6;
  //printf("tabW = %d\n", tabW);
  lineH = txthght * 1.2; ln_o = txthght * 0.83;
  tbrct.x = txthght * 2; tbrct.y = txthght * 2;
  tbrct.width = txthght * 24; tbrct.height = 12 * lineH;
  j = txthght * 0.62; // margin
  mgrct.x = tbrct.x - j; mgrct.y = tbrct.y - j;
  mgrct.width = tbrct.width + j * 2;
  mgrct.height = tbrct.height + j * 2;
                //Finish
  XMapWindow(dis, win);
  loop = true;
}

void shutdown() {
  XFree(atom_ret);
  XDestroyWindow(dis, sel_owner_win);
  XFreeCursor(dis, txtcursor);
  XShmDetach(dis, &shminfo);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);
  XDestroyWindow(dis, win);
}

int main() {
  init();
  if (!loop) return 1;
  char text;
  int endline;
  XEvent evnt;
  KeySym key;
  XConfigureEvent *xcfg;
  XSelectionRequestEvent *sev;
  XSelectionEvent ssev;
  while(loop) {
    pf = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyPress:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Control_L) lftctl = true;
        break;
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Escape || key == XK_q) {
          blinkeractive = false;
          loop = false;
	}
        else if (key == XK_Control_L) lftctl = false;
        else if (lftctl && key == XK_v && txtbxfocus  && !highlight) {
          if (select_own) getclipboard();
          else {
            operation = 2;
            XConvertSelection(dis, clpbrd, utf8str, prop, win, CurrentTime);
          }
        }
        else if (lftctl && key == XK_c && highlight) {
          operation = 1;
          XConvertSelection(dis, clpbrd, trgts, gdk_sel,
                   sel_owner_win, CurrentTime);
        }
        break;
      case ButtonPress:
        if (evnt.xbutton.button == 1) {
          if (txtbxfocus && mY > tbrct.y && mY < tbrct.y + tbrct.height) {
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
            highlight = false;
            if (redraw) drawtxtbx(true);
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
        break;
      case MotionNotify:
        mX = evnt.xbutton.x; mY = evnt.xbutton.y;
                    //section:  textbox focus
        if (mX > mgrct.x && mX < mgrct.x + mgrct.width && mY > mgrct.y
                && mY < mgrct.y + mgrct.height && !txtbxfocus) {
          txtbxfocus = true;
          if (!txtbxactive && redraw) drawtxtbx(true);
        }
        else if ((mX < mgrct.x || mX > mgrct.x + mgrct.width || mY < mgrct.y
                || mY > mgrct.y + mgrct.height) && txtbxfocus) {
          txtbxfocus = false;
          if (!txtbxactive && redraw) drawtxtbx(true);
        }
                    //section:  mouse over text
        if (doclength != 0) {
          if (mY < tbrct.y || mY > enddoc || nlines == 0) endline = tbrct.x;
          else endline = tbrct.x + ll[(mY - tbrct.y) / lineH];
          if (mY > tbrct.y && mY < enddoc && mX > tbrct.x
                  && mX < endline && !cursorcaret) {
            cursorcaret = true;
            XDefineCursor(dis, win, txtcursor);
          }
          else if ((mY < tbrct.y || mY > enddoc || mX < tbrct.x
                  || mX > endline) && cursorcaret) {
            cursorcaret = false;
            XDefineCursor(dis, win, None);
          }
        }
                    //section: mouse drag to highlight text
        if (mousedown && mY > tbrct.y && mY < enddoc && redraw) {
          findcrsrpos();
          if (caret > histcaret) {
            highlight = true;
            if (redraw) drawtxtbx(true);
          }
        }
        break;
      case Expose:
        if (redraw && (cfgW != bufW || cfgH != bufH)) resize_draw();
        pf = true; break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW) {
          blinkeractive = false;
          loop = false;
	}
        break;
      case SelectionClear:
        printf("lost ownership\n");
        select_own = false;
        clip.resize(1); clip[0] = 0;
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
	char *name = XGetAtomName(dis, sev->property);
        if (sev->property == None) {
          printf("Denying request '%s'\n", name);
        }
        else if (sev->target == trgts) {
          printf("Sending Atoms '%s'\n", name);
          XChangeProperty(dis, sev->requestor, sev->property,
                  XA_ATOM, 32, PropModeReplace, atom_ret, natoms);
        }
        else {
          printf("Sending data '%s'\n", name);
          XChangeProperty(dis, sev->requestor, sev->property,
                  utf8str, 8,  PropModeReplace,
                  reinterpret_cast<unsigned char*>(&clip[0]),
                  clip.size() - 1);
        }
        XFree(name);   
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
    if (pf) {
      rct.x = 0; rct.y = 0; rct.width = bufW; rct.height = bufH;
      paint();
    }
  }
  this_thread::sleep_for(chrono::milliseconds(50));
  shutdown();
  return 0;
}

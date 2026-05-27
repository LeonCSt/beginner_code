
/*  leon061.cpp:  launching a pop-up dialog box
    Calling  maind();  launches the dialog box
    Size of parent is fixed when child is mapped.
    g++ -I/usr/include/freetype2 leon061.cpp leon062.cpp -o leon -lX11 -lXext -lfreetype
                                                            [260524]  */

#include "leon060.h"
using namespace std;

// defining extern variables declared in leon060.h
Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XShmSegmentInfo shminfo;
FT_Library lbrry;
FT_Face face;
Cursor hndcursor;
unsigned *p;              // pixels
bool reply;
int txthght, mX, mY, bufW, bufH;                   
// end extern variables


Pixmap pxmp;
int waW, waH;             // working area width/height
int cfgW, cfgH;           // notified configuration W/H
XSizeHints sh;
XRectangle rct, btn;
unsigned char gct[256];   // gamma correction table
unsigned char prompt[] = "Click to open pop-up dialog";
bool redraw, loop, bf;   


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
  int i, j, k = 0, l, o;
  double r;
  fR = ((fg & 0xff0000) >> 16); fG = ((fg & 0xff00) >> 8);
  fB = (fg & 0xff); bR = ((bg & 0xff0000) >> 16);
  bG = ((bg & 0xff00) >> 8); bB = (bg & 0xff);
  j = Y + face->glyph->bitmap.rows;
  i = X + face->glyph->bitmap.width;
  for (Y; Y < j; Y++) {
    for (l = X; l < i; l++) {
      if (g[k] == 0) {}
      else if (g[k] == 255) {
        m[Y * bfW + l] = fg;
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
        m[Y * bfW + l] = d;
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
      X += 100 - X % 100;
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

int text_width(unsigned char* str, int length) {
  int pos = 0, num, i = 0;
  unsigned u, gi;
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
    else if (u < 32 || (u > 127 && u < 161)) {
      pos += num; continue;
    }
    else {
      gi = FT_Get_Char_Index(face, u);
      if (FT_Load_Glyph(face, gi, 0)) break;
      i += face->glyph->advance.x >> 6;
    }
    pos += num;
  }
  return i;
}

void launchd() {
  XDefineCursor(dis, win, None);
  sh.min_width = bufW; sh.min_height = bufH;
  sh.max_width = bufW; sh.max_height = bufH;
  XSetWMNormalHints(dis, win, &sh);
  maind();
}

void paint() {
  XCopyArea(dis, pxmp, win, gc, rct.x, rct.y,
          rct.width, rct.height, rct.x, rct.y);
  XFlush(dis);
  redraw = true;
}

void buttn(bool refresh) {
  redraw = false;
  if (bf) {
    fill_rectangle(p, bufW, bufH,
            btn.x, btn.y, btn.width, btn.height, 0xff333344);
    text_run(p, bufW, bufH, btn.x + txthght * 1.5, btn.y + txthght * 1.5,
            prompt, 27, 0xffcccc88);
  }
  else {
    fill_rectangle(p, bufW, bufH,
            btn.x, btn.y, btn.width, btn.height, 0xff222233);
    text_run(p, bufW, bufH, btn.x + txthght * 1.5, btn.y + txthght * 1.5,
            prompt, 27, 0xffaaaa66);
  }
  if (refresh) { rct = btn; paint(); }
}

void resize_draw() {
  redraw = false;
  bufW = cfgW; bufH =cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  int i, j;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000033;
  draw_rectangle(p, bufW, bufH, btn.x - 3, btn.y - 3,
          btn.width + 6, btn.height + 6, 0xffaaaa66);
  buttn(false);
}

void init() {
  dis = XOpenDisplay(0);
  waW = XDisplayWidth(dis, 0); waH = XDisplayHeight(dis, 0);
  txthght = 36; // <-- you can alter      range  10 -> 50
  if (txthght < 10 || txthght > 50) {
          printf("txthght: out of bounds\n"); loop = false; return; }
  cfgW = txthght * 27; cfgH = txthght * 15;
  gc = XDefaultGC(dis, DefaultScreen(dis));
                  //Section: Prep Window
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
  sh.flags = PPosition | PSize | PMinSize | PMaxSize |  PWinGravity;
  sh.x = left; sh.y = top; sh.width = cfgW; sh.height = cfgH;
  sh.max_width = waW; sh.max_height = waH; 
  sh.min_width = txthght * 19; sh.min_height = txthght * 10.7;
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
                  //Section: Set up gamma correction table
  double c, d;
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
  int j = 0;
  d = 255.5;
  for (int i = 255; i > -1; i--) {
    gct[i] = d;
    if (i % 16 == 15) {
       c = cv[j] / 16.0;
       j++;
    }
    d -= c;
  }
                  //Section: Set the size of elements
  btn.x = txthght * 2.5; btn.y = txthght * 2.5;
  btn.width = txthght * 14; btn.height = txthght * 2.5;
                  //Finish
  hndcursor = XCreateFontCursor(dis, XC_hand2);
  resize_draw();
  XMapWindow(dis, win);
  loop = true;
}

int main() {
  init();
  KeySym key;
  char text;
  bool pf;
  XEvent evnt;
  XConfigureEvent *xcfg;
  while(loop) {
    pf = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_Tab) { bf = !bf; buttn(true); }
        else if (key == XK_Return && bf) launchd();
        else if (key == XK_Escape || key == XK_q) loop = false;
        break;
      case ButtonPress:
        if (bf) launchd();
        break;
      case MotionNotify:
        mX = evnt.xbutton.x; mY = evnt.xbutton.y;
        if (bf) {
          if (mX < btn.x || mX > btn.x + btn.width ||
                  mY < btn.y || mY > btn.y + btn.height) {
            XDefineCursor(dis, win, None);
            bf = false; buttn(true);
          }
        }
        else if (mX > btn.x && mX < btn.x + btn.width &&
                mY > btn.y && mY < btn.y + btn.height) {
          XDefineCursor(dis, win, hndcursor);
          bf = true; buttn(true);
        }
        break;
      case Expose:
        pf = true; break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        if (redraw && (cfgW != bufW || cfgH != bufH))
                { resize_draw(); pf = true; }
        break;
      case DestroyNotify:
        sh.max_width = waW; sh.max_height = waH; 
        sh.min_width = txthght * 19; sh.min_height = txthght * 10.7;
        XSetWMNormalHints(dis, win, &sh);
        if (reply) printf("'X' is now --> 'On'\n");
        else printf("'X' is now --> 'Off'\n");
        bf = false; buttn(true);
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loop = false;
        break;
    }
    if (pf) {
      rct.x = 0; rct.y = 0; rct.width = bufW; rct.height = bufH;
      paint();
    }
  }
  XFreeCursor(dis, hndcursor);
  XShmDetach(dis, &shminfo);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);
  XDestroyWindow(dis, win);
}

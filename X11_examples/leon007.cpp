/*  leon007.cpp:  Text rendering with FreeType2
            
    g++ -I/usr/include/freetype2 leon007.cpp -o leon -lX11 -lXext -lfreetype
                                                               [260518]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <stdio.h>
#include <string>
using namespace std;

Display *dis;
GC gc;
Window win;
Atom WM_DELETE_WINDOW;
XShmSegmentInfo shminfo;
Pixmap pxmp;
FT_Library lbrry;
FT_Face face;
int waW, waH, bufW, bufH; // working area width/height   and  buffer W/H
int cfgW, cfgH;           // notified configuration W/H
int txthght, mX, mY;
unsigned char *b;         // bytes
unsigned *p;              // pixels
bool redraw, loop = true;
unsigned char gct[256];   // gamma correction table

void microview() {
  redraw = false;
  int src, dst, end, mvx, mvy;
  int i, j, k, l, m;
  src = (mY - 10) * bufW + mX - 8;
  mvx = (bufW - 192) / 2; mvy = bufH - 240;
  dst = mvy * bufW + mvx;
  end = bufW * bufH;
  l = 0;
  while (dst < end) {
    i = dst; j = dst + 192; k = 0; m = src;
    while (i < j) {
      p[i] = p[m];
      if (k % 12 == 10) {
        m++; i++; k++;
      }
      i++; k++;	    
    }
    if (l % 12 == 10) {
      dst += bufW; l++;
      src += bufW;
    }
    dst += bufW; l++;
  }
  XCopyArea(dis, pxmp, win, gc, mvx, mvy, 192, 240, mvx, mvy);
  XFlush(dis);
  redraw = true;
}

void draw_glyph(unsigned *m, int bufW, int bufH, int X, int Y,
        unsigned fg, unsigned bg) {
  if (Y < 0 || Y > bufH - face->glyph->bitmap.rows
          || X < 0 || X > bufW - face->glyph->bitmap.width) return;
  if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return;
  unsigned char fR, fG, fB, bR, bG, bB;
  unsigned char *b = &face->glyph->bitmap.buffer[0];
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
      if (b[k] == 0) {}
      else if (b[k] == 255) {
        m[Y * bufW + l] = fg;
      }
      else {
        d = 0xff000000;
        r = gct[b[k]] / 255.0;
        o = r * fR + (1 - r) * bR + 0.5;
        d += o << 16;
        o = r * fG + (1 - r) * bG + 0.5;
        d += o << 8;
        o = r * fB + (1 - r) * bB + 0.5;
        d += o;
        m[Y * bufW + l] = d;
      }
      k++;
    }
  }
}

void text_run(unsigned *m, int bufW, int bufH, int X, int Y,
        unsigned char *str, int length, unsigned fg) {
  int pos = 0, num;
  unsigned u, gi, bg;
  if (X > bufW - 50 || X < 0 || Y > bufH - 10 || Y < 30) return;
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
      X = 30; Y += txthght + 5;
      if (Y > bufH - 10) break;
    }
    else if(u == 9) {
      X += 100 - X % 100;
      if (X > bufW - 50) {
        X = 30; Y += txthght + 5;
        if (Y > bufH - 10) break;
      }
    }
    else if (u < 32 || (u > 127 && u < 161)) {
      pos += num; continue;
    }
    else {
      gi = FT_Get_Char_Index(face, u);
      if (FT_Load_Glyph(face, gi, 0)) break;
      if (face->glyph->bitmap.width != 0) {
        bg = p[(Y - (txthght / 3)) * bufW +
                X + ((face->glyph->advance.x >> 6) / 2)];
        draw_glyph(m, bufW, bufH, face->glyph->bitmap_left + X,
                Y - face->glyph->bitmap_top, fg, bg);
      }
      X += face->glyph->advance.x >> 6;
      if (X > bufW - 50) {
        X = 30; Y += txthght + 5;
        if (Y > bufH - 10) break;
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

void resize_draw() {
  redraw = false;
  bufW = cfgW; bufH =cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  unsigned d, c = 0xffaaaa66;
  int i, j, l, m, n, o;
  int k = (bufH / 2) * bufW + (bufW / 2);
  double r;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000033;
                //Section: Gradient
  j = (bufW - 256) / 2 - 30;
  m = (25 * bufW + j) * 4;
  for (i = 0; i < 60; i++) {
    for (n = 0; n < 256; n++) {
      b[m] = n;
      m += 4;
    }
    m += bufW * 4 - 1020;
  }
                //Section: text rendering
  unsigned char ttl[] = "Text Rendering";
  i = strlen(reinterpret_cast<const char*>(ttl));
  j = (bufW - text_width(ttl, i)) / 2;
  text_run(p, bufW, bufH, j, 65, ttl, i, c);
  unsigned char doc[] = "\tSimple rendering with extra strengthening on edge pixels (pseudo gamma correction). No kerning. Assuming 'flat' or gradient background. Does not offer texture or gradient foreground.\n\tUtf8 encoding L♡VE, j☺y, Pe☮ce.\n\tBecause it uses FreeType2, almost any level of complexity can be added to this basic approach..\n\tRegards ←";
  i = strlen(reinterpret_cast<const char*>(doc));
  text_run(p, bufW, bufH, 30, 130, &doc[0], i, c);
/* format: text_run(unsigned*       pointer to buffer begin in memory
                    int             buffer width
                    int             buffer height
                    int             penX,
                    int             penY,
                    unsigned char*  pointer to start position,
                    int             length of encoding in bytes,
                    unsigned        foregound color with alpha channel);  */

}

void paint() {
  XCopyArea(dis, pxmp, win, gc, 0, 0, bufW, bufH, 0, 0);
  XFlush(dis);
  redraw = true;
}

void init() {
  dis = XOpenDisplay(0);
  waW = XDisplayWidth(dis, 0); waH = XDisplayHeight(dis, 0);
                  //Section: Prep Window
  cfgW = 960; cfgH = 540;
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
  sh.min_width = 320; sh.min_height = 300;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  char snm[] = "Test GUI(Title Bar Text)";
  char *psnm = snm;
  XStringListToTextProperty(&psnm, 1, &nm);
  char sicnm[] = "Test GUI(Icon Text)";
  char *psicnm = sicnm;
  XStringListToTextProperty(&psicnm, 1, &icnm);
  XSetWMProperties(dis, win, &nm, &icnm, NULL, 0, &sh, &hnts, NULL);
                  //Section: Set up shared memory
  shminfo.readOnly = False;
  shminfo.shmid = shmget(IPC_PRIVATE, waW * waH * 4,
          IPC_CREAT | 0777);
  shminfo.shmaddr = reinterpret_cast<char*>(shmat(shminfo.shmid, 0, 0));
  b = reinterpret_cast<unsigned char*>(shminfo.shmaddr);
  p = reinterpret_cast<unsigned*>(shminfo.shmaddr);
  XShmAttach(dis, &shminfo);
                  //Section: Prep font
  FT_Init_FreeType(&lbrry);
  if (FT_New_Face(lbrry, "/usr/share/fonts/gnu-free/FreeSerif.otf",
          0, &face)) {printf("font not loaded\n"); loop = false; return;}
  printf("number of glyphs in this font = %d\n", face->num_glyphs);
  txthght = 28;
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
                  //Finish
  resize_draw();
  XMapWindow(dis, win);
  loop = true;
}

int main() {
  init();
  KeySym key;
  char text;
  bool pf;    // paint flag
  int mx, my;
  XEvent evnt;
  XConfigureEvent *xcfg;
  while(loop) {
    pf = false;
    XNextEvent(dis, &evnt);
    switch(evnt.type) {
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if ((key == XK_Escape) || (key == XK_q)) loop = false;
        //else printf("You pressed the %c key!\n",  text);
        break;
      case Expose:
        pf = true; break;
      case MotionNotify:
        mX = evnt.xbutton.x; mY = evnt.xbutton.y;
        if (redraw && mX > 7 && mX < bufW - 7  && mY > 9 && mY < bufH - 9)
                microview();	
        break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        if (redraw && (cfgW != bufW || cfgH != bufH)) resize_draw();
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

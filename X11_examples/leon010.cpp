/*  leon010.cpp:   Key/Mouse down driven animation.
            A W S D keys to move rectange. Mouse to move text.
    g++ -I/usr/include/freetype2 leon010.cpp -o leon -lX11 -lXext -lfreetype
                                                               [260520]  */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <thread>
#include <chrono>
#include <vector>
#include <stdio.h>
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
int txthght, offset, mv;
atomic<int> mX, mY;
unsigned char *b;         // bytes
unsigned *p;              // pixels
atomic<bool> redraw = true, loop;
atomic<bool> kydwn, msdwn, rdrw, tdrw;
atomic<bool> ky[4]; // AWSD key flags
unsigned char gct[256];   // gamma correction table
XRectangle rrec, trec, rhst, thst;
vector <XRectangle> rct;
unsigned char txt[] = "Some Text";

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

void resize_draw() {
  redraw = false;
  bufW = cfgW; bufH =cfgH;
  if (pxmp) XFreePixmap(dis, pxmp);
  pxmp = XShmCreatePixmap(dis, win, shminfo.shmaddr, &shminfo,
          bufW, bufH, 24);
  int i, j;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000033;
  rrec.x = txthght * 7; rrec.y = txthght * 3.5;
  trec.x = txthght * 3; trec.y = txthght * 1.2;
  fill_rectangle(p, bufW, bufH,
          rrec.x, rrec.y, rrec.width, rrec.height, 0xff444444);
  text_run(p, bufW, bufH, trec.x, trec.y + offset, txt, 9, 0xffaaaa66);
  rhst = rrec; thst = trec;
}

void paint() {
  for (int i = 0; i < rct.size(); i++) {
    XCopyArea(dis, pxmp, win, gc, rct[i].x, rct[i].y,
            rct[i].width, rct[i].height, rct[i].x, rct[i].y);
  }
  XFlush(dis);
  rct.clear();
  redraw = true;
}

void draw_animat() {
  redraw = false; 
  if (tdrw) {
    fill_rectangle(p, bufW, bufH,
            thst.x, thst.y, thst.width, thst.height, 0xff000033);
    rct.push_back(thst);
    thst = trec;
  }
  if (rdrw) {
    fill_rectangle(p, bufW, bufH,
            rhst.x, rhst.y, rhst.width, rhst.height, 0xff000033);
    rct.push_back(rhst);
    rhst = rrec;
    fill_rectangle(p, bufW, bufH,
            rrec.x, rrec.y, rrec.width, rrec.height, 0xff444444);
    rct.push_back(rrec);
  } 
  if (tdrw) {
    text_run(p, bufW, bufH, trec.x, trec.y + offset, txt, 9, 0xffaaaa66);
    rct.push_back(trec);
  }
  paint();
}

void animate() {
  int x, y, w, h;
  w = 0.5 * trec.width; h = 0.5 * trec.height;
  while (loop) {
    if ((kydwn || msdwn) && redraw) {
      tdrw = false; rdrw = false;
      if (kydwn) {
        if (ky[0] && !ky[1] && rrec.x < bufW - rrec.width - mv) {
                rrec.x += mv; rdrw = true;}
        if (ky[1] && !ky[0] && rrec.x > mv) { rrec.x -= mv; rdrw = true; }
        if (ky[2] && !ky[3] && rrec.y < bufH - rrec.height - mv) {
                rrec.y += mv; rdrw = true;}
        if (ky[3] && !ky[2] && rrec.y > mv) { rrec.y -= mv; rdrw = true; }
      }
      if (msdwn) {
        x = mX - w; y = mY - h;
        if (x < bufW - trec.width && x > 0 && y < bufH - trec.height
                && y > 0 && (x != trec.x || y != trec.y)) {
          trec.x = x; trec.y = y; tdrw = true;
        }
      }
                    //hit testing rectangles
      if (rdrw && !tdrw) {
        if ((rhst.x < trec.x + trec.width && rhst.x + rhst.width > trec.x
                && rhst.y < trec.y + trec.height
                && rhst.y + rhst.height > trec.y)
                || (rrec.x < trec.x + trec.width
                && rrec.x + rrec.width > trec.x
                && rrec.y < trec.y + trec.height
                && rrec.y + rrec.height > trec.y)) {
          tdrw = true;
        }
      }
      if (!rdrw && tdrw) {
        if ((thst.x < rrec.x + rrec.width && thst.x + thst.width > rrec.x
                && thst.y < rrec.y + rrec.height
                && thst.y + thst.height > rrec.y )
                || (trec.x < rrec.x + rrec.width
                && trec.x + trec.width > rrec.x
                && trec.y < rrec.y + rrec.height
                && trec.y + trec.height > rrec.y )) {
          rdrw = true;
        }
      }
      if (tdrw || rdrw) { thread t2(draw_animat); t2.detach(); }
    }   
    this_thread::sleep_for(chrono::milliseconds(30));
  }
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
  sh.min_width = 672; sh.min_height = 378;
  sh.win_gravity = CenterGravity;
  hnts.flags = 1; hnts.input = true;
  XStoreName(dis, win, "Test GUI(Title Bar Text)");
  XSetIconName(dis, win, "Test GUI(Icon Text)");
  XSetWMNormalHints(dis, win, &sh);
  XSetWMHints(dis, win, &hnts);
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
  //printf("number of glyphs in this font = %d\n", face->num_glyphs);
  txthght = 36;  // <--  you can alter     range 10 -> 50
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
                  //Set sizes of elements
  rrec.width = txthght * 5; rrec.height = txthght * 1.5;
  trec.width = txthght * 4.7; trec.height = txthght * 1.3;
  offset = txthght * 0.87; mv = txthght * 0.25;
  rct.reserve(5);
                  //Finish
  XMapWindow(dis, win);
  loop = true;
}

int main() {
  init();
  if (!loop) return 1;
  thread t1(animate); t1.detach();
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
      case KeyPress:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_d || key == XK_a || key == XK_s || key == XK_w) {
          if (key == XK_d) ky[0] = true;
          else if (key == XK_a) ky[1] = true;
          else if (key == XK_s) ky[2] = true;
          else ky[3] = true;
          kydwn = true;
        }
        break;
      case KeyRelease:
        XLookupString(&evnt.xkey, &text, 1, &key, 0);
        if (key == XK_d || key == XK_a || key == XK_s || key == XK_w) {
          if (key == XK_d) ky[0] = false;
          else if (key == XK_a) ky[1] = false;
          else if (key == XK_s) ky[2] = false;
          else ky[3] = false;
          if (kydwn && !ky[0] && !ky[1] && !ky[2] && !ky[3]) kydwn = false;
        }
        else if ((key == XK_Escape) || (key == XK_q)) loop = false;
        break;
      case ButtonPress:
        if (evnt.xbutton.button == 1) msdwn = true;
        break;
      case ButtonRelease:
        if (evnt.xbutton.button == 1) msdwn = false;
        break;
      case Expose:
        if (redraw && (cfgW != bufW || cfgH != bufH)) resize_draw();
        pf = true; break;
      case MotionNotify:
        mX = evnt.xbutton.x; mY = evnt.xbutton.y;
        break;
      case ConfigureNotify:
        xcfg = reinterpret_cast<XConfigureEvent*>(&evnt);
        cfgW = xcfg->width; cfgH = xcfg->height;
        break;
      case ClientMessage:
        if ((Atom) evnt.xclient.data.l[0] == WM_DELETE_WINDOW)
                loop = false;
        break;
    }
    if (pf) {
      rct.resize(1);
      rct[0].x = 0; rct[0].x = 0;
      rct[0].width = bufW; rct[0].height = bufH;
      paint();
    }
  }
  XShmDetach(dis, &shminfo);
  shmdt(shminfo.shmaddr);
  shmctl(shminfo.shmid, IPC_RMID, 0);
  XDestroyWindow(dis, win);
  return 0;
}


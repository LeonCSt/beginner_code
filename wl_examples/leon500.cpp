/*  leon500.cpp | Using the wayland clipboard.
      [Ctrl] [C]  to cut      [Ctrl] [V]  to paste
      hold[Ctrl] [C]then[V]  to copy
        Textbox starts out empty so you have to copy
        from somewhere else then paste in to get started.
        Example -- copy --> "Utf8 encoding L♡VE, j☺y, Pe☮ce."
        (In gvim it is [Shift] ['][=]  then  [Y] to copy to clipboard.)
      Use mouse to highlight or place caret in text.
        (Not implemented: Keyboard keys to highlight or move caret.)
      [Q] or [ESC] to close
    *needs*  xdg-shell-protocol.cpp       | see wl_examplesREADME.txt
             xdg-shell-client-protocol.h  |

    g++ -I/usr/include/freetype2 xdg-shell-protocol.cpp leon500.cpp -o leon -lwayland-client -lwayland-cursor -lfreetype
       Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./leon
       or - - - - -   prefix with the following for debug   - - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . . 
    g++ -fsanitize=thread -g -O1 . . .
                                                      [251130] */

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include "xdg-shell-client-protocol.h"
using namespace std;

wl_display *dis;
wl_registry *reg;
wl_output *out;
wl_compositor *comp;
wl_shm *shm;
xdg_wm_base *xdgwmb;
wl_data_device_manager *datadman;
wl_seat *seat;
wl_surface *surf;
xdg_surface *xdgsurf;
xdg_toplevel *xdgtop;
wl_buffer *buff;
wl_shm_pool *pool;
wl_cursor_theme *crsr_theme;
wl_cursor *crsr;
wl_surface *crsr_surf;
wl_buffer *crsr_buff;
wl_cursor_image *crsr_img;
wl_keyboard *kybrd;
wl_pointer *pntr;
wl_data_device *datdvc;
wl_data_source *datsrc;
wl_data_offer *dta_offr, *hst_offr;
FT_Library lbrry;
FT_Face face;
unsigned pntr_serial, kbrd_serial;
unsigned *p; // pixel array for window
vector<unsigned char> clp = {0};
vector<unsigned char> doc = {0};
vector<unsigned> ls = {0}; // ls[]  each lines start pos within doc[]
vector<unsigned short> ll = {0}; //  ll[]   pixel length of each line
int txtH, mX, mY, lftM, topM;
int doclength, caret, nlines, clp_size, enddoc;
int tbX, tbY, tbW, tbH, tabW, lineH, ln_o;
int caretx, carety, histcaret, histcaretx, histcarety;
int monW, monH, surfW, surfH, bufW, bufH;
atomic <bool> evloop = true, redraw = true, blinkeractive;
bool loopkeep, paintflag, txtbxfocus, cursorcaret, txtbxactive;
bool blinker, mousedown, highlight, select_own, clpbrd, lftctl;
unsigned char gct[256];  // gamma correction table


void draw_rectangle(int X, int Y, int width, int height, unsigned fg) {
  if (X < 0 || X > bufW - width
            || Y < 0 || Y > bufH - height) return;
  int g, h, j, i, k;
  k = Y * bufW + X; h = (Y + height - 1) * bufW + X;
  for (i = 0; i < width; i++) {
    p[k] = fg; p[h] = fg;
    k += 1; h += 1;
  }
  g = height - 2; k = (Y + 1) * bufW + X;
  h = (Y + 1) * bufW + X + width - 1;
  for (i = 0; i < g; i++) {
    p[k] = fg; p[h] = fg;
    k += bufW; h += bufW;
  }
}

void fill_rectangle(int X, int Y, int width, int height, unsigned fg) {
  if (X < 0 || X > bufW - width
            || Y < 0 || Y > bufH - height) return;
  int h, i, j, k;
  if ((fg & 0xff000000) == 0xff000000) {
    h = Y * bufW + X;
    j = Y + height; k = X + width;
    for (Y; Y < j; Y++) {
      for (i = X; i < k; i++) {
       p[h] = fg;
       h++;
      }
    h += bufW - width;
    }
  }
  else {
    // implement transparancy
  }
}

void draw_glyph(int X, int Y, unsigned fg, unsigned bg) {
  if (Y < 0 || Y > bufH - face->glyph->bitmap.rows
          || X < 0 || X > bufW - face->glyph->bitmap.width) return;
  if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return;
  unsigned char fR, fG, fB, bR, bG, bB;
  unsigned char *b = &face->glyph->bitmap.buffer[0];
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
      if (b[k] == 0) {}
      else if (b[k] == 255) {
        p[Y * bufW + n] = fg;
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
        p[Y * bufW + n] = d;
      }
      k++;
    }
  }
}

void text_run(int X, int Y, unsigned char *str, int length,
        unsigned fg, unsigned bg) {
  int pos = 0, num, i, j, k;
  unsigned u, gi;
  i = txtH * face->ascender / 1152 + 1;
  j = txtH * face->descender / -1152 + 1;
  k = txtH * 0.63;
  if (X > bufW - k || X < 0 || Y > bufH - j || Y < i) return;
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
      X = tbX; Y += txtH + 5;
      if (Y > bufH - 10) break;
    }
    else if(u == 9) {
      X += tabW - (X - lftM) % tabW;
      if (X > bufW - txtH * 0.8 + 0.5) {
        X = tbX; Y += txtH + 5;
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
        draw_glyph(face->glyph->bitmap_left + X,
                Y - face->glyph->bitmap_top, fg, bg);
      }
      X += face->glyph->advance.x >> 6;
      if (X > bufW - txtH * 0.8 + 0.5) {
        X = tbX; Y += txtH + 5;
        if (Y > bufH - 10) break;
      }
    }
    pos += num;
  }
}

void set_cursor() {
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_damage(crsr_surf, 0, 0, crsr_img->width, crsr_img->height);
  wl_surface_commit(crsr_surf);
}

void draw_textbox(bool refresh) {
  redraw = false;
  int i, j, k;
  unsigned fg = 0xffaaaa66, bg;
  if (txtbxfocus || txtbxactive) bg = 0xff171739; else bg = 0xff09092a;
  fill_rectangle(tbX + 1, tbY + 1, tbW - 2, tbH - 2, bg);
  if (doclength > 0) {
    j = topM + ln_o;
    for (i = 0; i < nlines; i++) {
      if (i == 12) break;
      if (!highlight || ls[i + 1] <= histcaret || ls[i] >= caret) {
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(lftM, j, &doc[ls[i]], k, fg, bg);
      }
      else if (histcaret <= ls[i] && caret >= ls[i + 1]) {
        fill_rectangle(lftM, j - ln_o, ll[i], lineH, fg);
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(lftM, j, &doc[ls[i]], k, bg, fg);
      }
      else if (histcaret <= ls[i] && caret < ls[i + 1]) {
        fill_rectangle(lftM, j - ln_o, caretx - lftM, lineH, fg);
        k = caret - ls[i];
        text_run(lftM, j, &doc[ls[i]], k, bg, fg);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(caretx, j, &doc[caret], k, fg, bg);
      }
      else if (histcaret > ls[i] && caret >= ls[i + 1]) {
        k = histcaret - ls[i];
        text_run(lftM, j, &doc[ls[i]], k, fg, bg);
        fill_rectangle(histcaretx, histcarety,
                ll[i] + lftM - histcaretx, lineH, fg);
        k = ls[i + 1] - histcaret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(histcaretx, j, &doc[histcaret],
                k, bg, fg);
      }
      else {
        k = histcaret - ls[i];
        text_run(lftM, j, &doc[ls[i]], k, fg, bg);
        fill_rectangle(histcaretx, histcarety,
                caretx - histcaretx, lineH, fg);
        k = caret - histcaret;
        text_run(histcaretx, j, &doc[histcaret], k, bg, fg);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        text_run(caretx, j, &doc[caret], k, fg, bg);
      }
      j += lineH;
    }
  }
  if (refresh) {
    wl_surface_attach(surf, buff, 0, 0);
    wl_surface_damage(surf, tbX + 1, tbY + 1, tbW - 2, tbH - 2);
    wl_surface_commit(surf); 
  }
}

void castlines() {
  int pos = 0, num, tally = 0, space, spcpos, histpos = 0;
  unsigned u, gi;
  if (nlines != 0) {
    ls.erase(ls.begin(), ls.begin() + nlines);
    ll.erase(ll.begin(), ll.begin() + nlines);
    nlines = 0;
  }
  ls[0] = doclength;
  while (pos < doclength) {
    if (pos == caret) {
      caretx = tally + lftM;
      carety = topM + nlines * lineH;
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
    if (tally > 27 * txtH) {
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
        caretx = tally + lftM;
        carety = topM + nlines * lineH;
      }
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      nlines ++;
    }
    pos += num;
  }
  //printf("number of lines = %d\n", nlines);
  if (nlines > 12) {
    if (caret > ls[12]) {
      caret = ls[12];
      carety = topM + 11 * lineH;
      caretx = lftM + ll[11];
    }
    enddoc = topM + 12 * lineH;
  }
  else enddoc = topM + nlines * lineH;
  draw_textbox(true);
}

void findcrsrpos() {
  int pos, num, tally = lftM;
  unsigned u, gi;
  if (mY > topM + nlines * lineH) {
    caret = doclength;
    caretx = ll[nlines - 1] + lftM;
    carety = topM + (nlines - 1) * lineH;
  }
  else {
    int j = (mY - topM) / lineH;
    carety = topM + j * lineH;
    if (mX - lftM > ll[j]) {
      caret = ls[j + 1];
      caretx = ll[j] + lftM;
    }
    else if (mX < lftM) {
      caret = ls[j];
      caretx = lftM;
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
        if (doc[pos] == 9) tally += tabW - (tally - lftM) % tabW;
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

void caretblinker() {
  blinker = true;
  int i = 0, j, k, x, y;
  unsigned h[lineH];
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
      wl_surface_attach(surf, buff, 0, 0);
      wl_surface_damage(surf, x, y, 1, lineH);
      wl_surface_commit(surf);
      wl_display_flush(dis);
      blinker = !blinker;
    }
    this_thread::sleep_for(chrono::milliseconds(23));
    i++;
  }
  if (evloop && redraw) draw_textbox(true);
}

void top_config(void *data, xdg_toplevel *xdgtop, int width,
        int height, wl_array *states) {
  surfW = width; surfH = height;
}

void top_close(void *data, xdg_toplevel *xdgtop) {
  blinkeractive = false; evloop = false;
}

//void top_config_bounds(void *data, xdg_toplevel *xdgtop,
//        int32_t width, int32_t height) {
//}

void top_wm_capabils(void *data, xdg_toplevel *xdgtop,
        wl_array *caps) {
}

xdg_toplevel_listener top_lstnr = {
  .configure = top_config,
  .close = top_close,
//  .configure_bounds = top_config_bounds,
  .wm_capabilities = top_wm_capabils,
};

void data_o_offer(void *data, wl_data_offer *data_offer,
        const char *mime_type) {
  //printf("mime_type data offer = %s\n", mime_type);
}

void data_o_source_actions(void *data, wl_data_offer *data_offer,
        unsigned source_actions) {
  printf("source_actions data offer = %u\n", source_actions);
}

void data_o_action(void *data, wl_data_offer *data_offer,
        unsigned dnd_action) {
  printf("dnd_action data offer = %u\n", dnd_action);
}

wl_data_offer_listener dat_offer_lstnr = {
  .offer = data_o_offer,
  .source_actions = data_o_source_actions,
  .action = data_o_action,
};

void dat_src_target(void *data, wl_data_source *data_source,
        const char *mime_type) {
  printf("mim_type data source = %s\n", mime_type);
}

void dat_src_send(void *data, wl_data_source *data_source,
        const char *mime_type, int32_t fd) {
  if (strcmp(mime_type, "UTF8_STRING") == 0) {
    printf("piping to clipboard\n");
    write(fd, &clp[0], clp_size);
    close(fd);
  } else printf("Requesting a different mime_type -- %s\n", mime_type);
}

void dat_src_cancelled(void *data, wl_data_source *data_source) {
  select_own = false;
  wl_data_source_destroy(datsrc);
  datsrc = NULL;
  printf("someone else now owns the clipboard\n");
}

wl_data_source_listener dat_src_lstnr = {
  .target = dat_src_target,
  .send = dat_src_send,
  .cancelled = dat_src_cancelled,
};

void receive_clpbrd() {
  redraw = false;
  int i, j = 0, size = 4096;
  if (!select_own && (dta_offr != hst_offr)) {
                //Section: Setting up the pipe
    printf("piping now\n");
    int fds[2];
    pipe(fds);
    wl_data_offer_receive(dta_offr, "text/plain", fds[1]);
    close(fds[1]);
    wl_display_roundtrip(dis);
                //Section: Getting the data
    while (size == 4096) {
      unsigned char temp[4096];
      size = read(fds[0], temp, sizeof(temp));
      clp.resize(j + size + 1);
      for (i = 0; i < size; i++) {
        clp[j] = temp[i];
        j++;
      }
    }
    close(fds[0]);
    hst_offr = dta_offr;
    clp[j] = 0;
    clp_size = j;
  }
                //Section: Inserting the clip
  printf("inserting\n");
  doc.resize(doclength + clp_size + 1);
  for (i = doclength; i >= caret; i--) doc[i + clp_size] = doc[i];
  j = caret;
  for (i = 0; i < clp_size; i++) { doc[j] = clp[i]; j++; }
  caret += clp_size; doclength += clp_size;
  castlines();
}

void send_to_clpbrd() {
  redraw = false;
  if (select_own)
          wl_data_device_set_selection(datdvc, NULL, kbrd_serial);
  wl_display_roundtrip(dis);
  int i, j, k;
  j = caret - histcaret; k = histcaret;
  clp.resize(j + 1); clp_size = j;
  for (i = 0; i < j; i++) { clp[i] = doc[k]; k++; }
  clp[i] = 0;
  for (i = caret; i <= doclength; i++) doc[i - j] = doc[i];
  doclength -= j; caret -= j;
  doc.resize(doclength + 1);
  if (doclength == 0) { txtbxactive = false; blinkeractive = false; }
  printf("notify of your data source offer\n");
  if (datsrc) { wl_data_source_destroy(datsrc); datsrc = NULL; }
  datsrc = wl_data_device_manager_create_data_source(datadman);
  wl_data_source_add_listener(datsrc, &dat_src_lstnr, NULL);
  wl_data_source_offer(datsrc, "UTF8_STRING");
  wl_data_device_set_selection(datdvc, datsrc, kbrd_serial);
  select_own = true; highlight = false;
  castlines();
}

void datdev_data_offer(void *data, wl_data_device *data_device,
        wl_data_offer *id) {
  wl_data_offer_add_listener(id, &dat_offer_lstnr, NULL);
}

void datdev_selection(void *data, wl_data_device *data_device,
        wl_data_offer *id) {
  if (dta_offr) {
    wl_data_offer_destroy(dta_offr);
    dta_offr = NULL; hst_offr = NULL;
  }
  if (id) {
    dta_offr = id;
    clpbrd = true;
    printf("clipboard active\n");
  } else {
    clpbrd = false;
    printf("clipboard empty\n");
  }
}

wl_data_device_listener dat_dev_lstnr = {
  .data_offer = datdev_data_offer,
  .selection = datdev_selection,
};


void kybrd_keymap(void *data, wl_keyboard *kybrd, unsigned format,
        int fd, unsigned size) {
}

void kybrd_enter(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf, wl_array *keys) {
  kbrd_serial = serial;
}

void kybrd_leave(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf) {
}

void kybrd_key(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned time, unsigned key, unsigned state) {
     /*    _ESC 1,   _Q 16,   _C 46,   _V 47,   _LEFTCTRL 29,  */
  if (state == 1) {
    if (key == 29) lftctl = true;
  }
  else {
    if (key == 29) lftctl = false;
    else if (key == 16 || key == 1) {
            blinkeractive = false; evloop = false; }
    else if (redraw && lftctl && key == 47 &&
            clpbrd && txtbxfocus && !highlight) receive_clpbrd();
    else if (redraw && lftctl && key == 46 && highlight)
            send_to_clpbrd();
  }
}

void kybrd_modifiers(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned mods_depressed, unsigned mods_latched,
        unsigned mods_locked, unsigned group) {
}

void kybrd_repeat(void *data, wl_keyboard *kybrd, int rate,
        int delay) {
}

wl_keyboard_listener kybrd_lstnr = {
  .keymap = kybrd_keymap,
  .enter = kybrd_enter,
  .leave = kybrd_leave,
  .key = kybrd_key,
  .modifiers = kybrd_modifiers,
  .repeat_info = kybrd_repeat,
};

void pntr_enter(void *data, wl_pointer *pntr, unsigned serial,
        wl_surface *surface, wl_fixed_t surf_x, wl_fixed_t surf_y) {
  pntr_serial = serial;
  crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  set_cursor();
}

void pntr_leave(void *datia, wl_pointer *pntr, unsigned serial,
        wl_surface *surface) {
}

void pntr_motion(void *data, wl_pointer *pntr, unsigned time,
        wl_fixed_t surf_x, wl_fixed_t surf_y) {
  mX = wl_fixed_to_int(surf_x); mY = wl_fixed_to_int(surf_y);
  int endline;
                //Section: textbox focus
  if (mX > tbX && mX < tbX + tbW && mY > tbY
          && mY < tbY + tbH && !txtbxfocus) {
    txtbxfocus = true;
    if (!txtbxactive && redraw) { draw_textbox(true); return; }
  }
  else if ((mX < tbX || mX > tbX + tbW || mY < tbY
                      || mY > tbY + tbH) && txtbxfocus) {
    txtbxfocus = false;
    if (!txtbxactive && redraw) { draw_textbox(true); return; }
  }
                //section:  mouse over text
  if (doclength != 0) {
    if (mY < topM || mY > enddoc || nlines == 0) endline = lftM;
    else endline = lftM + ll[(mY - topM) / lineH];
    if (mY > topM && mY < enddoc && mX > lftM
                  && mX < endline && !cursorcaret) {
      cursorcaret = true;
      crsr = wl_cursor_theme_get_cursor(crsr_theme, "xterm");
      set_cursor();
    }
    else if ((mY < topM || mY > enddoc || mX < lftM
                        || mX > endline) && cursorcaret) {
      cursorcaret = false;
      crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
      set_cursor();
    }
  }
                //section: mouse drag to highlight text
  if (mousedown && mY > topM && mY < enddoc && redraw) {
    if (!redraw) return;
    findcrsrpos();
    if (caret > histcaret) highlight = true; else highlight = false;
    if (redraw) draw_textbox(true);
  }
}

void pntr_button(void *data, wl_pointer *pntr, unsigned serial,
        unsigned time, unsigned button, unsigned state) {
        // BTN_LEFT 272,  BTN_RIGHT 273,  BTN_MIDDLE 274
  if (doclength == 0) return;
  if (state == 1) { //Section: Button Down
    if (button == 272) {
      highlight = false;
      if (txtbxfocus && mY > topM && mY < enddoc && redraw) {
        blinkeractive = false;
        findcrsrpos();
        histcaret = caret;
        histcaretx = caretx; histcarety = carety;
        mousedown = true;
        txtbxactive = true;
        this_thread::sleep_for(chrono::milliseconds(23));
      }
      else {
        txtbxactive = false;
        blinkeractive = false;
        if (redraw) draw_textbox(true);
      }
    }
  }
  else {            //Section: Button Up
    if (button == 272 && txtbxactive) {
      mousedown = false;
      if (caret > histcaret) highlight = true;
      else highlight = false;
      blinkeractive = true;
      thread t1(caretblinker); t1.detach();
    }
  }
}

void pntr_axis(void *data, wl_pointer *pntr, unsigned time,
        unsigned axis, wl_fixed_t value) {
}

void pntr_frame(void *data, wl_pointer *pntr) {
}

void pntr_axis_source(void *data, wl_pointer *pntr,
        unsigned axis_source) {
}

//void pntr_axis_stop(void *data, wl_pointer *pntr, uint32_t time,
//        uint32_t axis) {
//}

//void pntr_axis_discrete(void *data, wl_pointer *pntr, uint32_t axis,
//        int32_t discrete) {
//}

void pntr_axis_value120(void *data, wl_pointer *pntr, unsigned axis,
        int value120) {
}

void pntr_axis_rel_dir(void *data, wl_pointer *pntr, unsigned axis,
        unsigned direction) {
}

wl_pointer_listener pntr_lstnr = {
  .enter = pntr_enter,
  .leave = pntr_leave,
  .motion = pntr_motion,
  .button = pntr_button,
  .axis = pntr_axis,
  .frame = pntr_frame,
  .axis_source = pntr_axis_source,
//  .axis_stop = pntr_axis_stop,
//  .axis_discrete = pntr_axis_discrete,
  .axis_value120 = pntr_axis_value120,
  .axis_relative_direction = pntr_axis_rel_dir,
};

void buff_release(void *data, wl_buffer *wl_buffer) {
  redraw = true;
}

wl_buffer_listener buff_listnr = {
  .release = buff_release,
};

void draw_resize() {
  redraw = false;
  bufW = surfW; bufH = surfH;
  if (buff) wl_buffer_destroy(buff); 
  buff = wl_shm_pool_create_buffer(pool, 0, bufW, bufH,
          bufW * 4, WL_SHM_FORMAT_XRGB8888);
  int i, j;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
  draw_rectangle(tbX, tbY, tbW, tbH, 0xffaaaa66);
  draw_textbox(false);
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_damage(surf, 0, 0, bufW, bufH);
  wl_surface_commit(surf);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfW != bufW || surfH != bufH)) {
    thread t1(draw_resize); t1.detach();
    xdg_surface_ack_configure(xdgsurf, serial);
  }
}

xdg_surface_listener xdgsrf_lstnr = {
  .configure = xdgsurf_cnfgr,
};

void xdgwmb_ping(void *data, xdg_wm_base *xdgwmb, unsigned serial) {
  xdg_wm_base_pong(xdgwmb, serial);
}

xdg_wm_base_listener wmb_lstnr = {
  .ping = xdgwmb_ping,
};

void output_geometry(void* data, wl_output *wl_output,
        int x, int y, int physical_width,
        int physical_height, int subpixel, const char *make,
        const char *model, int transform) {
}

void output_mode(void *data, wl_output *wl_output,
        unsigned flags, int width, int height, int refresh) {
  monW = width; monH = height;
}

void output_done(void *data, wl_output *wl_output) {
}

void output_scale(void *data, wl_output *wl_output,
        int factor) {
}

void output_name(void *data, wl_output *wl_output,
        const char *name) {
}

void output_description(void *data,
        wl_output *wl_output, const char *description) {
}

wl_output_listener out_lstnr = {
  .geometry = output_geometry,
  .mode = output_mode,
  .done = output_done,
  .scale = output_scale,
  .name = output_name,
  .description = output_description,
};

void registry_global(void *data, wl_registry *regi,
        unsigned name, const char *interface, unsigned version) {
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    comp = (wl_compositor*)wl_registry_bind(
            regi, name, &wl_compositor_interface, version);
  }
  else if (strcmp(interface, wl_output_interface.name) == 0) {
    out = (wl_output*)wl_registry_bind(
            regi, name, &wl_output_interface, version);
  }
  else if (strcmp(interface, wl_shm_interface.name) == 0) {
    shm = (wl_shm*)wl_registry_bind(
            regi, name, &wl_shm_interface, version);
  }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    xdgwmb = (xdg_wm_base*)wl_registry_bind(
            regi, name, &xdg_wm_base_interface, version);
  }
  else if (strcmp(interface, wl_seat_interface.name) == 0) {
    seat = (wl_seat*)wl_registry_bind(
            regi, name, &wl_seat_interface, version);
  }
  else if (strcmp(interface,
          wl_data_device_manager_interface.name) == 0) {
    datadman = (wl_data_device_manager*)wl_registry_bind(
            regi, name, &wl_data_device_manager_interface, version);
  }
}

void registry_global_remove(void *data,
        wl_registry *reg, unsigned name) {
  printf("Global Object listed as '%u' is removed.\n", name);
}

wl_registry_listener rgstry_lstnr = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

int main() {
  dis = wl_display_connect(NULL);
  reg = wl_display_get_registry(dis);
  wl_registry_add_listener(reg, &rgstry_lstnr, NULL);
  wl_display_roundtrip(dis);
                //Section: Prep Window
  wl_output_add_listener(out, &out_lstnr, NULL);
  surf = wl_compositor_create_surface(comp);
  xdg_wm_base_add_listener(xdgwmb, &wmb_lstnr, NULL);
  xdgsurf = xdg_wm_base_get_xdg_surface(xdgwmb, surf);
  xdg_surface_add_listener(xdgsurf, &xdgsrf_lstnr, NULL);
  xdgtop = xdg_surface_get_toplevel(xdgsurf);
  xdg_toplevel_add_listener(xdgtop, &top_lstnr, NULL);
  wl_display_roundtrip(dis);
  xdg_toplevel_set_max_size(xdgtop, monW, monH);
  xdg_toplevel_set_min_size(xdgtop, 832, 468);
  xdg_toplevel_set_title(xdgtop,"Test GUI");
  wl_surface_commit(surf);
  pntr = wl_seat_get_pointer(seat);
  wl_pointer_add_listener(pntr, &pntr_lstnr, NULL);
  kybrd = wl_seat_get_keyboard(seat);
  wl_keyboard_add_listener(kybrd, &kybrd_lstnr, NULL);
  datdvc = wl_data_device_manager_get_data_device(datadman, seat);
  wl_data_device_add_listener(datdvc, &dat_dev_lstnr, NULL);
                //Section: Prep Shared memory Buffer
  int fd = syscall(SYS_memfd_create, "shm-leon", 0);  //<sys/syscall.h>
  int size = monW * monH * 4;                     //<unistd.h>
  ftruncate(fd, size);
  p = (unsigned*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
          fd, 0);                                     //<sys/mman.h>
  pool = wl_shm_create_pool(shm, fd, size);
  close(fd);
                //Section: Prep Cursors
  crsr_theme = wl_cursor_theme_load(nullptr, 24, shm);
  crsr_surf = wl_compositor_create_surface(comp);
                //Section: Prep font
  FT_Init_FreeType(&lbrry);
  if (FT_New_Face(lbrry, "/usr/share/fonts/gnu-free/FreeSerif.otf",
          0, &face)) {printf("font not loaded\n"); return 1;}
  //printf("number of glyphs in this font = %d\n", face->num_glyphs);
  txtH = 28;
  FT_Set_Pixel_Sizes(face, 0, txtH);
                //Section: Make gamma correction table
  int i, j = 0;
  double c, d;
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
  d = 255.5;
  for (i = 255; i > -1; i--) {
    gct[i] = d;
    if (i % 16 == 15) {
       c = cv[j] / 16.0;
       j++;
    }
    d -= c;
  }
                //Section: Show window
  surfW = 864; surfH = 486;
  FT_Load_Glyph(face, FT_Get_Char_Index(face, 48), 0);
  tabW = 8 * face->glyph->advance.x >> 6;
  tbX = txtH; tbY = txtH;
  tbW = txtH * 29; tbH = txtH * 16.5 + 0.5; 
  lftM = 2 * txtH; topM = txtH * 1.6 + 0.5;
  lineH = 1.25 * txtH + 0.5; ln_o = 0.85 * txtH + 0.5;
  draw_resize();
  while (wl_display_dispatch(dis) != -1) {
    if (!evloop) break;
  }
  this_thread::sleep_for(chrono::milliseconds(30));
  if (hst_offr) wl_data_offer_destroy(hst_offr);
  if (datsrc) wl_data_source_destroy(datsrc);
  if (datdvc) wl_data_device_destroy(datdvc);
  if (kybrd) wl_keyboard_destroy(kybrd);
  if (pntr) wl_pointer_destroy(pntr);
  if (seat) wl_seat_destroy(seat);
  if (crsr_theme) wl_cursor_theme_destroy(crsr_theme);
  if (crsr_surf) wl_surface_destroy(crsr_surf);
  if (xdgtop) xdg_toplevel_destroy(xdgtop);
  if (xdgsurf) xdg_surface_destroy(xdgsurf);
  if (surf) wl_surface_destroy(surf);
  if (buff) wl_buffer_destroy(buff);
  if (xdgwmb) xdg_wm_base_destroy(xdgwmb);
  if (shm) wl_shm_destroy(shm);
  if (comp) wl_compositor_destroy(comp);
  if (out) wl_output_destroy(out);
  if (reg) wl_registry_destroy(reg);
  if (pool) wl_shm_pool_destroy(pool);
  if (p) munmap(p, size);
  if (face) FT_Done_Face(face);
  if (lbrry) FT_Done_FreeType(lbrry);
  wl_display_disconnect(dis);
}


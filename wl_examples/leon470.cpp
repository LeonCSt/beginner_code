/*  leon470.cpp | render *.otf and *.ttf fonts
      use  fc-list | grep "\.otf"  or  "\.ttf"  to see your system fonts
      modify line 526 to change fonts
      [Q] or [ESC] to close

    *needs*  xdg-shell-protocol.cpp  and  xdg-shell-client-protocol.h
             see wl_examplesREADME.txt

    COMPILE : -
    g++ $(pkg-config --cflags freetype2 ..etc..) your_source.cpp -o your_program $(pkg-config --libs -lfreetype ..etc..)

    g++ -I/usr/include/freetype2 xdg-shell-protocol.cpp leon470.cpp -o leon -lwayland-client -lwayland-cursor -lfreetype
      Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./leon                      - - - - - -   prefix with the following for debug   - - - - - -
    g++ -fsanitize=address -fno-omit-frame-pointer -g -O1 . . . 
    g++ -fsanitize=thread -g -O1 . . .
                                                      [251027] */

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
#include <atomic>
#include "xdg-shell-client-protocol.h"
using namespace std;

wl_display *dis;
wl_registry *reg;
wl_output *out;
wl_compositor *comp;
wl_shm *shm;
xdg_wm_base *xdgwmb;
wl_seat *seat;
wl_surface *surf, *crsr_surf;
xdg_surface *xdgsurf;
xdg_toplevel *xdgtop;
wl_shm_pool *pool;
wl_buffer *buff, *crsr_buff;
wl_cursor_theme *crsr_theme;
wl_pointer *pntr;
wl_cursor *crsr;
wl_cursor_image *crsr_img;
wl_keyboard *kybrd;
FT_Library lbrry;
FT_Face face;
unsigned *p; // pixel array for window
atomic <bool> loop = true, redraw = true, kydwn = false;
int monW, monH;
int surfW, surfH;
int bufW, bufH;
int txtH, mX, mY;
unsigned char gct[256];  // gamma correction table

void microview() {
  redraw = false;
  int src, dst, end, mvx, mvy;
  int i, j, k, m, n;
  src = (mY - 10) * bufW + mX - 8;
  mvx = (bufW - 192) / 2; mvy = bufH - 240;
  dst = mvy * bufW + mvx;
  end = bufW * bufH;
  n = 0;
  while (dst < end) {
    i = dst; j = dst + 192; k = 0; m = src;
    while (i < j) {
      p[i] = p[m];
      if (k % 12 == 10) {
        m++; i++; k++;
      }
      i++; k++;	    
    }
    if (n % 12 == 10) {
      dst += bufW; n++;
      src += bufW;
    }
    dst += bufW; n++;
  }
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_damage(surf, mvx, mvy, 192, 240);
  wl_surface_commit(surf);
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
        unsigned fg) {
  int pos = 0, num, i;
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
      X = 30; Y += txtH + 5;
      if (Y > bufH - 10) break;
    }
    else if(u == 9) {
      X += 100 - X % 100;
      if (X > bufW - 50) {
        X = 30; Y += txtH + 5;
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
        bg = p[(Y - (txtH / 3)) * bufW +
                X + ((face->glyph->advance.x >> 6) / 2)];
        draw_glyph(face->glyph->bitmap_left + X,
                Y - face->glyph->bitmap_top, fg, bg);
      }
      X += face->glyph->advance.x >> 6;
      if (X > bufW - 50) {
        X = 30; Y += txtH + 5;
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

void top_config(void *data, xdg_toplevel *xdgtop, int width,
        int height, wl_array *states) {
  surfW = width; surfH = height;
}

void top_close(void *data, xdg_toplevel *xdgtop) {
  loop = false;
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

void kybrd_keymap(void *data, wl_keyboard *kybrd, unsigned format,
        int fd, unsigned size) {
}

void kybrd_enter(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf, wl_array *keys) {
}

void kybrd_leave(void *data, wl_keyboard *kybrd, unsigned serial,
        wl_surface *surf) {
}

void kybrd_key(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned time, unsigned key, unsigned state) {
  if (state == 1) kydwn = true;
  else {
    kydwn = false;
    if (key == 16 || key == 1) loop = false;
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
        wl_surface *surf, wl_fixed_t surf_x, wl_fixed_t surf_y) {
  crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_commit(crsr_surf);
}

void pntr_leave(void *data, wl_pointer *pntr, unsigned serial,
        wl_surface *surf) {
}

void pntr_motion(void *data, wl_pointer *pntr, unsigned time,
        wl_fixed_t surf_x, wl_fixed_t surf_y) {
  mX = wl_fixed_to_int(surf_x); mY = wl_fixed_to_int(surf_y);
  if (!kydwn && redraw && mX > 7 && mX < bufW - 8
            && mY > 9 && mY < bufH - 10) {
    thread t2(microview); t2.detach();
  } 
}

void pntr_button(void *data, wl_pointer *pntr, unsigned serial,
        unsigned time, unsigned button, unsigned state) {
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

void draw() {
  redraw = false;
  bufW = surfW; bufH = surfH;
  if (buff) wl_buffer_destroy(buff);
  buff = wl_shm_pool_create_buffer(pool, 0, bufW, bufH,
          bufW * 4, WL_SHM_FORMAT_XRGB8888);
  int i, j, m, n;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
                //Section: Gradient
  j = (bufW - 256) / 2 - 30;
  m = (25 * bufW + j);
  for (i = 0; i < 60; i++) {
    for (n = 0; n < 256; n++) {
      p[m] = n;
      m++;
    }
    m += bufW - 255;
  }
                //Section: text rendering
  unsigned char ttl[] = "Text Rendering";
  i = strlen(reinterpret_cast<const char*>(ttl));
  j = (bufW - text_width(ttl, i)) / 2;
  text_run(j, 65, ttl, i, 0xffaaaa66);
  unsigned char doc[] = "\tSimple rendering with extra strengthening on edge pixels (pseudo gamma correction). No kerning. Assuming 'flat' background. Does not offer texture or gradient foreground.\n\tUtf8 encoding L♡VE, j☺y, Pe☮ce.\n\tBecause it uses FreeType2, almost any level of complexity can be added to this basic approach..\n\tRegards ←";
  i = strlen(reinterpret_cast<const char*>(doc));
  text_run(30, 120, &doc[0], i, 0xffaaaa66);
/* format: text_run(int             penX,
                    int             penY,
                    unsigned char*  pointer to start position,
                    int             length of encoding in bytes,
                    unsigned        foregound color); */
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfW != bufW || surfH != bufH)) {
    thread t1(draw); t1.detach();
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
                //Section: make table
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
  draw();
  while (wl_display_dispatch(dis) != -1) {
    if (!loop) break;
  }
  this_thread::sleep_for(chrono::milliseconds(30));
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
  wl_shm_pool_destroy(pool);
  munmap(p, size); p = NULL;
  if (face) FT_Done_Face(face);
  if (lbrry) FT_Done_FreeType(lbrry);
  wl_display_disconnect(dis);
}


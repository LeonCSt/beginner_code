/*  leon491.cpp | Launch a popup dialog.
      Get user's input for a simple On or Off 'setting'.
      [Tab] [Enter] keyboard interact, or use mouse.
      [Q] or [ESC] to close
    Dialog is launched by calling  maind()  from anywhere in this file.
    Execution resumes in  dialog_return()  when popup is closed.

    *needs*  leon490.h    | the header file for extern variables
             leon492.cpp  | the file that handles the popup dialog

    *needs*  xdg-shell-protocol.cpp       | see wl_examplesREADME.txt
             xdg-shell-client-protocol.h  |

    g++ -I/usr/include/freetype2 xdg-shell-protocol.cpp leon491.cpp leon492.cpp -o leon -lwayland-client -lwayland-cursor -lfreetype
                                                       [251113] */

#include "leon490.h"
using namespace std;

            //Section: defining extern variables declared in leon490.h
wl_display *dis;
wl_compositor *comp;
wl_shm *shm;
xdg_wm_base *xdgwmb;
wl_seat *seat;
wl_surface *d_surf;
xdg_surface *xdgsurf;
wl_cursor_theme *crsr_theme;
wl_cursor *crsr;
FT_Face face;
int surfwidth, surfheight;
int txthght, mX, mY, d_mx, d_my;
bool setting, popfocus, popup;
unsigned pntr_serial, d_key, d_key_state;
unsigned d_pntr_btn, d_pntr_btn_state;
            //End Section

wl_output *out;
wl_registry *reg;
wl_surface *surf;
xdg_toplevel *xdgtop;
wl_shm_pool *pool;
wl_buffer *buff;
wl_surface *crsr_surf;
wl_buffer *crsr_buff;
wl_cursor_image *crsr_img;
wl_pointer *pntr;
wl_keyboard *kybrd;
FT_Library lbrry;
unsigned *p; // pixel array for window
bool xit = false, redraw = true, btn_focus = false;
int hrdwrX, hrdwrY;
int bffwdth, bffhght;
int btnX, btnY, btnW, btnH;
unsigned char doc[] = "Click to open pop-up dialog";

void launch_popup() {
  xdg_toplevel_set_min_size(xdgtop, bffwdth, bffhght);
  xdg_toplevel_set_max_size(xdgtop, bffwdth, bffhght);
  btn_focus = false;
  maind();
}

void draw_rectangle(unsigned *m, int bW, int bH, int X, int Y,
        int width, int height, unsigned fg) {
  if (X < 0 || X > bW - width
            || Y < 0 || Y > bH - height) return;
  int g, h, j, i, k;
  k = Y * bW + X; h = (Y + height - 1) * bW + X;
  for (i = 0; i < width; i++) {
    m[k] = fg; m[h] = fg;
    k += 1; h += 1;
  }
  g = height - 2; k = (Y + 1) * bW + X;
  h = (Y + 1) * bW + X + width - 1;
  for (i = 0; i < g; i++) {
    m[k] = fg; m[h] = fg;
    k += bW; h += bW;
  }
}

void fill_rectangle(unsigned *m, int bW, int bH, int X, int Y,
        int width, int height, unsigned fg) {
  if (X < 0 || X > bW - width
            || Y < 0 || Y > bH - height) return;
  int h, i, j, k;
  if ((fg & 0xff000000) == 0xff000000) {
    h = Y * bW + X;
    j = Y + height; k = X + width;
    for (Y; Y < j; Y++) {
      for (i = X; i < k; i++) {
       m[h] = fg;
       h++;
      }
    h += bW - width;
    }
  }
  else {
    // implement transparancy
  }
}

void draw_glyph(unsigned *m, int bW, int bH, int X, int Y,
        unsigned fg, unsigned bg, unsigned char* gc) {
  if (Y < 0 || Y > bH - face->glyph->bitmap.rows
          || X < 0 || X > bW - face->glyph->bitmap.width) return;
  if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return;
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
        m[Y * bW + l] = fg;
      }
      else {
        d = 0xff000000;
        r = gc[b[k]] / 255.0;
        o = r * fR + (1 - r) * bR + 0.5;
        d += o << 16;
        o = r * fG + (1 - r) * bG + 0.5;
        d += o << 8;
        o = r * fB + (1 - r) * bB + 0.5;
        d += o;
        m[Y * bW + l] = d;
      }
      k++;
    }
  }
}

void text_run(unsigned *m, int bW, int bH, int X, int Y,
        unsigned char *str, int length, unsigned fg, unsigned bg) {
  int pos = 0, num, i, j = 0;
  unsigned u, gi;
  double c, d;
  unsigned char gc[256];  // gamma correction table
  if (X > bW - txthght * 0.8 + 0.5 || X < 0
          || Y > bH - txthght * 0.3 + 0.5 || Y < txthght * 0.8 + 0.5)
          return;
                    //Section: make table
  unsigned char cv[] = {5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20,
                        23, 27, 32, 39}; //curve
  d = 255.5;
  for (i = 255; i > -1; i--) {
    gc[i] = d;
    if (i % 16 == 15) {
       c = cv[j] / 16.0;
       j++;
    }
    d -= c;
  }
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
      if (Y > bH - 10) break;
    }
    else if(u == 9) {
      X += 100 - X % 100;
      if (X > bW - txthght * 0.8 + 0.5) {
        X = 30; Y += txthght + 5;
        if (Y > bH - 10) break;
      }
    }
    else if (u < 32 || (u > 127 && u < 161)) {
      pos += num; continue;
    }
    else {
      gi = FT_Get_Char_Index(face, u);
      if (FT_Load_Glyph(face, gi, 0)) break;
      if (face->glyph->bitmap.width != 0) {
        draw_glyph(&m[0], bW, bH, face->glyph->bitmap_left + X,
                Y - face->glyph->bitmap_top, fg, bg, &gc[0]);
      }
      X += face->glyph->advance.x >> 6;
      if (X > bW - txthght * 0.8 + 0.5) {
        X = 30; Y += txthght + 5;
        if (Y > bH - 10) break;
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

void set_cursor() {
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_commit(crsr_surf);
}

void draw_button() {
  unsigned bg, fg = 0xffaaaa66;
  if (btn_focus) bg = 0xff222277; else bg = 0xff000022;
  int i = strlen(reinterpret_cast<const char*>(doc));
  fill_rectangle(&p[0], bffwdth, bffhght, btnX + 1, btnY + 1,
          btnW - 2, btnH - 2, bg);
  text_run(&p[0], bffwdth, bffhght, btnX + 20,
          btnY + txthght * 1.08 + 0.5, &doc[0], i, fg, bg);
}

void refresh_button() {
  redraw = false;
  draw_button();
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_damage(surf, btnX + 1, btnY + 1, btnW - 2, btnH - 2);
  wl_surface_commit(surf);
}

void dialog_return() {
  popup = false; popfocus = false;
  if (setting) printf("'X' setting is set to 'ON'\n");
  else printf("'X' setting is set to 'OFF'\n");
  xdg_toplevel_set_min_size(xdgtop, 864, 468);
  xdg_toplevel_set_max_size(xdgtop, hrdwrX, hrdwrY);
  refresh_button();
  wl_display_flush(dis);
}

void top_config(void *data, xdg_toplevel *xdgtop, int width,
        int height, wl_array *states) {
  surfwidth = width; surfheight = height;
}

void top_close(void *data, xdg_toplevel *xdgtop) {
  xit = true;
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
  if (popfocus) {
    d_key = key; d_key_state = state;
    thread t5(d_kbd_key); t5.detach();
    return;
  }
         /*    _ESC 1,  _TAB 15,  _Q 16,  _ENTER 28,    */
  if (state == 1) {
  }
  else {
    if (key == 16 || key == 1) { xit = true; return; }
    if (popup) return;
    if (key == 15) {
      btn_focus = !btn_focus;
      if (redraw) refresh_button();
    }
    else if (key == 28 && btn_focus) {
      thread t3(launch_popup); t3.detach();
    }
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
  if (surface == d_surf) popfocus = true; else popfocus = false;
  crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  set_cursor();
}

void pntr_leave(void *data, wl_pointer *pntr, unsigned serial,
        wl_surface *surface) {
  if (surface == d_surf) popfocus = false;
}

void pntr_motion(void *data, wl_pointer *pntr, unsigned time,
        wl_fixed_t surf_x, wl_fixed_t surf_y) {
  mX = wl_fixed_to_int(surf_x); mY = wl_fixed_to_int(surf_y);
  if (popfocus) {
    d_mx = mX; d_my = mY;
    thread t2(d_pntr_motion); t2.detach();
    return;
  }
  if (popup) return;
  bool pflag = false;
  if (btn_focus) {
    if ((mX < btnX) || (mX >= (btnX + btnW)) ||
            (mY < btnY) || (mY >= (btnY + btnH))) {
      btn_focus = false; pflag = true;
      crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
      set_cursor();
    }
  }
  else if ((mX > btnX) && (mX < (btnX + btnW)) &&
         (mY > btnY) && (mY < (btnY + btnH))) {
    btn_focus = true; pflag = true;
    crsr = wl_cursor_theme_get_cursor(crsr_theme, "hand2");
    set_cursor();
  }
  if (pflag && redraw) refresh_button();
}

void pntr_button(void *data, wl_pointer *pntr, unsigned serial,
        unsigned time, unsigned button, unsigned state) {
  if (popfocus) {
    d_pntr_btn = button; d_pntr_btn_state = state;
    thread t4(d_pntr_button); t4.detach();
    return;
  }
  if (popup) return;
  if (state == 1) {}
  else {
    if (btn_focus) {
      thread t3(launch_popup); t3.detach();
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
  bffwdth = surfwidth; bffhght = surfheight;
  buff = wl_shm_pool_create_buffer(pool, 0, bffwdth, bffhght,
          bffwdth * 4, WL_SHM_FORMAT_XRGB8888);
  unsigned fg = 0xffaaaa66, bg = 0xff000022;
  int i, j;
  j = bffwdth * bffhght;
  for (i = 0; i < j; i++) p[i] = bg;
  draw_rectangle(&p[0], bffwdth, bffhght, btnX, btnY, btnW, btnH, fg);
  draw_button();
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfwidth != bffwdth || surfheight != bffhght)) {
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
  hrdwrX = width; hrdwrY = height;
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
  xdg_toplevel_set_max_size(xdgtop, hrdwrX, hrdwrY);
  xdg_toplevel_set_min_size(xdgtop, 832, 468);
  xdg_toplevel_set_title(xdgtop,"Test GUI");
  wl_surface_commit(surf);
  pntr = wl_seat_get_pointer(seat);
  wl_pointer_add_listener(pntr, &pntr_lstnr, NULL);
  kybrd = wl_seat_get_keyboard(seat);
  wl_keyboard_add_listener(kybrd, &kybrd_lstnr, NULL);
                //Section: Prep Shared memory Buffer
  int fd = syscall(SYS_memfd_create, "shm-leon", 0);  //<sys/syscall.h>
  int size = hrdwrX * hrdwrY * 4;                     //<unistd.h>
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
  txthght = 28;
  FT_Set_Pixel_Sizes(face, 0, txthght);
                //Section: Show window
  surfwidth = 864; surfheight = 486;
  int i = strlen(reinterpret_cast<const char*>(doc));
  int j = text_width(&doc[0], i);
  btnX = txthght * 3; btnY = txthght * 2;
  btnW = j + 40; btnH = txthght * 1.6 + 0.5;
  draw_resize();
  while (wl_display_dispatch(dis) != -1) {
    if (xit) break;
  }
  if (popup) d_close();
  wl_shm_pool_destroy(pool);
  wl_display_disconnect(dis);
}

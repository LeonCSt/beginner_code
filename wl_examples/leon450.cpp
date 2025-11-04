/*  leon450.cpp | Add pointer. Use the system cursors.
      Window appears, accepts keyboard, mouse, and resize events.
      mouseclick to toggle through cursors
      [Q] or [ESC] to close

    *needs*  xdg-shell-protocol.cpp  and  xdg-shell-client-protocol.h
             see wl_examplesREADME.txt

    g++ xdg-shell-protocol.cpp leon450.cpp -o leon -lwayland-client -lwayland-cursor
                                                            [251003] */

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <cstring>
#include <string>
#include <thread>
#include <math.h>
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
unsigned *p; // pixel array for window
bool xit = false, redraw = true;
int hrdwrX, hrdwrY;
int surfwidth, surfheight;
int bffwdth, bffhght;
unsigned pntr_serial;
int pcrsr = 0; //present cursor

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
  if (key == 16 || key == 1) xit = true;
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
  pntr_serial = serial;
  crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_commit(crsr_surf);
  pcrsr = 0;
}

void pntr_leave(void *data, wl_pointer *pntr, unsigned serial,
        wl_surface *surf) {
}

void pntr_motion(void *data, wl_pointer *pntr, unsigned time,
        wl_fixed_t surf_x, wl_fixed_t surf_y) {
}

void pntr_button(void *data, wl_pointer *pntr, unsigned serial,
        unsigned time, unsigned button, unsigned state) {
  if (state == 1) return;
  pcrsr++;
  if (pcrsr == 3) pcrsr = 0;
  string cn = "";
  if (pcrsr == 0) cn = "left_ptr";
  else if (pcrsr == 1) cn = "xterm";
  else cn = "hand2";
  crsr = wl_cursor_theme_get_cursor(crsr_theme, cn.c_str());
  crsr_img = crsr->images[0];
  crsr_buff = wl_cursor_image_get_buffer(crsr_img);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr_img->hotspot_x, crsr_img->hotspot_y);
  wl_surface_attach(crsr_surf, crsr_buff, 0, 0);
  wl_surface_commit(crsr_surf);
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
  bffwdth = surfwidth; bffhght = surfheight;
  buff = wl_shm_pool_create_buffer(pool, 0, bffwdth, bffhght,
          bffwdth * 4, WL_SHM_FORMAT_XRGB8888);
  unsigned c = 0xffaaaa66;
  int i, j, m, n, o;
  int k = (bffhght / 2) * bffwdth + (bffwdth / 2);
  double r;
  for (i = 0; i < (bffwdth * bffhght); i++) p[i] = 0xff000022;
            //Section: Circle
  if (bffwdth <= bffhght) r = bffwdth * 0.45;
  else r = bffhght * 0.45;
  o = 0.7071 * r + 1.5;
  for (i = 0; i < o; i++) {
    j = (cos(asin(i / r)) * r + 0.5);
    n = i * bffwdth; m = j * bffwdth;
    p[k + i + m] = c; p[k + i - m] = c;
    p[k - i + m] = c; p[k - i - m] = c;
    p[k + j + n] = c; p[k + j - n] = c;
    p[k - j + n] = c; p[k - j - n] = c;
  }
            //Section: black and white squares
  m = bffwdth - 200;
  n = 21 * bffwdth - 220;
  o = (bffhght - 219) * bffwdth - 220; 
  for (j = 0; j < 200; j++) {
    for (i = 0; i < 200; i++) {
      p[n] = 0xffffffff; p[o] = 0xff000000;
      n++; o++;
    }
    n += m; o +=m;
  }
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfwidth != bffwdth || surfheight != bffhght)) {
    xdg_surface_ack_configure(xdgsurf, serial);
    thread t1(draw); t1.detach();
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
          fd, 0); //<sys/mman.h>
  pool = wl_shm_create_pool(shm, fd, size);
  close(fd);
                //Section: Prep Cursors
  crsr_theme = wl_cursor_theme_load(nullptr, 24, shm);
  crsr_surf = wl_compositor_create_surface(comp);
                //Section: Show window
  surfwidth = 864; surfheight = 486;
  draw();
  while (wl_display_dispatch(dis) != -1) {
    if (xit) break;
  }
  wl_shm_pool_destroy(pool);
  wl_display_disconnect(dis);
}

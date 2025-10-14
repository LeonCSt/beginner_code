/*  leon460.cpp | Self made cursors.
    Window appears, accepts keyboard, mouse, and resize events.
      [Q] or [ESC] to close
      Cursors are generated and stored in the main shm_pool.
      Enlarge to show cursor foundry. Mouseclick to toggle cursor.
    
    g++ xdg-shell-protocol.cpp leon460.cpp -o leon -lwayland-client
                                                          [250911]  */

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <wayland-client.h>
#include <cstring>
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
wl_pointer *pntr;
wl_keyboard *kybrd;
unsigned *p; // pixel array for window and cursors
bool xit = false, redraw = true;
int hrdwrX, hrdwrY;
int surfwidth, surfheight;
int bffwdth, bffhght;
double crsrscale = 1.0;  //range 0.5 to 2.0
    // crsr[] Format: {offset,   wdth, hght, htX, htY,   then
    //                 scaled--> wdth, hght, htX, htY }
int crsr[] = {0, 14, 22, 0, 0, 14, 22, 0, 0, // arrow|
        4928, 11, 22, 5, 11, 11, 22, 5, 11,  // caret|
        9856, 17, 18, 6, 0, 17, 18, 6, 0};   // hand |
unsigned pntr_serial;
int pcrsr = 0; //present cursor
bool crsr_invert = false;
unsigned f[19712]; // cursor foundry space
int fwdth, fhght;

void scale_cursors() {
  int i, j, k, l;
  for (i = 1; i < 20; i += 9) {
    j = crsrscale * crsr[i] * 4.0 + 0.5;
    k = j % 4;
    if (k != 0) j = j - k + 4;
    crsr[i + 4] = j / 4.0;
    j = crsrscale * crsr[i + 1] * 4.0 + 0.5;
    k = j % 4;
    if (k != 0) j = j - k + 4;
    crsr[i + 5] = j / 4.0;
    crsr[i + 6] = crsrscale * crsr[i + 2] + 0.5;
    crsr[i + 7] = crsrscale * crsr[i + 3] + 0.5;
  }
}

void cast_cursor(int num, int X, int Y) {
  unsigned d;
  int c, i, j, k, l, m, o;
  double r;
  c = hrdwrX * hrdwrY + num * 1232;
  l = crsr[num * 9 + 6] * 10 + Y;
  for (j = Y; j < l; j++) {
    m = c;
    k = crsr[num * 9 + 5] * 10 + X;
    for (i = X; i < k; i++) {
      d = 0xff000000;
      r = 1 - ((p[m] & 0xff000000) >> 24) / 255.0;
      o = ((p[m] & 0xff0000) >> 16) + r *
              ((p[j * bffwdth + i] & 0xff0000) >> 16) + 0.5;
      d += o * 0x10000;
      o = ((p[m] & 0xff00) >> 8) + r *
              ((p[j * bffwdth + i] & 0xff00) >> 8) + 0.5;
      d += o * 0x100;
      o = (p[m] & 0xff) + r * (p[j * bffwdth + i] & 0xff) + 0.5;
      d += o;
      p[j * bffwdth + i] = d;
      if (i % 10 == 9) m++;
    }
    if (j % 10 == 9) c += crsr[num * 9 + 5];
  }
}

void condense_image(int offset) {
     // 16 subpixels are mixed into a final pixel
  int i, j, k, l, m, n, o, q;
  unsigned char ca, cr, cg, cb;
  double a, r, g, b, d;
  k = offset;
  o = fwdth * fhght;
  for (j = 0; j < o; j += (4 * fwdth)) {
    n = j + fwdth;
    for (i = j; i < n; i += 4) {
      a = 0; r = 0; g = 0; b = 0;
      q = i + 4 * fwdth;
      for (m = i; m < q; m += fwdth) {
        for (l = m; l < (m + 4); l++) {
          a += ((f[l] & 0xff000000) >> 24);
          r += ((f[l] & 0xff0000) >> 16);
          g += ((f[l] & 0xff00) >> 8);
          b += (f[l] & 0xff);
        }
      }
      a /= 16.0; r /= 16.0; g /= 16.0; b /= 16.0;
      a += 0.5; r += 0.5; g += 0.5; b += 0.5;
      ca = a; cr = r; cg = g; cb = b;
      p[k] = ca * 0x1000000 + cr * 0x10000 + cg * 0x100 + cb;
      k++;
    }
  }
}

void drawline(int iniX, int iniY, int destX, int destY,
        unsigned color) {
  int s, rx, ry;
  double x, y, tx, ty;
  tx = iniX + 0.5; ty = iniY + 0.5;
  if (pow((destY - iniY), 2) <= pow((destX - iniX), 2)) {
    if (destX >= iniX) {
      s = destX - iniX;
      x = 1.0;
    }
    else {
      s = iniX - destX;
      x = -1.0;
    }
    y = (destY - iniY) / (double)s;
  }
  else {
    if (destY >= iniY) {
      s = destY - iniY;
      y = 1.0;
    }
    else {
      s = iniY - destY;
      y = -1.0;
    }
    x = (destX - iniX) / (double)s;
  }
  for (int i = 1; i <= s; i++) {
    tx += x; ty += y;
    rx = tx; ry = ty;
    f[ry * fwdth + rx] = color;
  }
}

void closed_path_fill(int *q, int length, unsigned color) {
  unsigned c;
  bool d = true, h;
  int i, j, k, l, m, n;
            //Section: get temporary random color
  while (d) {
    c = (rand() % 0x1000000) + 0xff000000;
    if (c == color) continue;
    d = false;
    for (i = 0; i < 19712; i++) {
      if (f[i] == c) d = true;
    }
  }
            //Section: draw outline
  for (i = 0; i < (length - 2); i += 2) {
    j = q[i] * crsrscale + 0.5;
    k = q[i + 1] * crsrscale + 0.5;
    l = q[i + 2] * crsrscale + 0.5;
    m = q[i + 3] * crsrscale + 0.5;
    drawline(j, k, l,  m, c);
    if ((i < length - 4) && (q[i + 4] == -1)) i += 3;
  }

  // return;

            //Section: fill
  i = fwdth; j = 1;
  while (j < (fhght - 1)) {
    l = i; k = 0; h = false;
    while (k < fwdth) {
      d = false;
      while (f[l] != c) {
        l++; k++;
        if (k == fwdth) {
          d = true;
          break;
        }
      }
      if (d) break;
      if (k > 0) {
        m = l + fwdth - 1;
      }
      else m = i + fwdth;
      d = true;
      while (d) {
        if (f[l] != c) {
          d = false; break;
        }
        l++; k++;
        if (k == fwdth) break;
      }
      if (d) break;
      n = l + fwdth;
      while (m <= n) {
        if (f[m] == c) d = true;
        m++;
      }
      if (d) h = !h;
      m = k; n = l; d = false;
      if (h) {
        while (f[n] != c) {
          if ((m > fwdth) ||
                  ((f[n - fwdth] != c) && (f[n - fwdth] != color))) {
            d = true; break;
          }
          m++; n++;
        }
      }
      else {
        while (f[n] != c) {
          if (m > fwdth) break;
          if (f[n - fwdth] == color) {
            d = true; break;
          }
          m++; n++;
        }
      }
      if (d) h = !h;
      if (h) {
        while (f[l] != c) {
          if (k > fwdth) break;
          f[l] = color;
          l++; k++;
        }
      }
      k++;
    }
    j++; i += fwdth;
  }
            //Section: recolor outline to color
  j = fwdth * fhght;
  for (i = 0; i < j; i++) {
    if (f[i] == c) f[i] = color;
  }
}

void cursors_draw() {
  int i, j = hrdwrX * hrdwrY;
  unsigned a, b;
  for (i = j; i < j + 3696; i++) p[i] = 0;
  if (crsr_invert) {
    a = 0xff000000; b = 0xffffffff;
  }
  else {
    a = 0xffffffff; b = 0xff000000;
  }
  srand(time(NULL));
//Format:         q[] = {<closed path>, -1, <closed path>, . . .}
//Format: <closed path> {startX, startY, x, y, x, y, . . stopX, stopY}
//             stopX must = startX      stopY must = startY
//        int q[] = {7, 8, 50, 3, 48, 70, 12, 75, 7, 8, -1, 20, 20,
//                   40, 17, 43, 60, 23, 63, 20, 20};
//        closed_path_fill(&q[0], 21, a);
            //Section: Caret
  for (i = 0; i < 19712; i++) f[i] = 0;
  fwdth = crsr[14] * 4; fhght = crsr[15] * 4;
  int k[] = {0, 0, 41, 0, 41, 17, 29, 17, 29, 68,
             41, 68, 41, 85, 0, 85, 0, 68, 12, 68,
             12, 17, 0, 17, 0, 0};
  closed_path_fill(&k[0], 26, a);
  int l[] = {6, 6, 18, 6, 21, 9, 24, 6, 35, 6,
             35, 11, 28, 11, 23, 16, 23, 69, 28, 74,
             35, 74, 35, 79, 24, 79, 21, 76, 18, 79,
             6, 79, 6, 74, 13, 74, 18, 69, 18, 16,
             13, 11, 6, 11, 6, 6};
  closed_path_fill(&l[0], 46, b);
  condense_image(j + 1232);
            //Section: Hand
  for (i = 0; i < 19712; i++) f[i] = 0;
  fwdth = crsr[23] * 4; fhght = crsr[24] * 4;
  int g[] = {24, 69, 0, 45, 0, 31, 3, 28, 8, 28,
             16, 36, 16, 5, 21, 0, 28, 0, 33, 5,
             33, 20, 40, 19, 45, 24, 52, 23, 56, 28,
             64, 29, 67, 33, 67, 39, 57, 69, 24, 69};
  closed_path_fill(&g[0], 40, a);
  int c[] = {25, 63, 6, 44, 6, 38, 8, 36, 19, 47,
             22, 45, 22, 6, 27, 6, 27, 20, 31, 26,
             38, 25, 43, 30, 50, 29, 55, 34, 61, 36,
             61, 39, 53, 63, 25, 63};
  closed_path_fill(&c[0], 36, b);
  condense_image(j + 2464);
            //Section: Arrow
  for (i = 0; i < 19712; i++) f[i] = 0;
  fwdth = crsr[5] * 4; fhght = crsr[6] * 4;
  int q[] = {0, 0, 3, 0, 55, 39, 55, 42, 34, 44,
             48, 72, 47, 78, 45, 81, 42, 83, 38, 84,
             33, 84, 30, 81, 16, 53, 0, 69, 0, 0};
  closed_path_fill(&q[0], 30, a);
  int r[] = {6, 10, 43, 38, 24, 39, 42, 72, 42, 76,
             40, 78, 35, 78, 33, 76, 17, 44, 6, 55, 6, 10};
  closed_path_fill(&r[0], 22, b);
  condense_image(j);
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
  crsr_buff = wl_shm_pool_create_buffer(pool, crsr[0], crsr[5],
          crsr[6], crsr[5] * 4, WL_SHM_FORMAT_ARGB8888);
  wl_pointer_set_cursor(pntr, serial, crsr_surf, crsr[7], crsr[8]);
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
  crsr_buff = wl_shm_pool_create_buffer(pool, crsr[pcrsr * 9],
          crsr[pcrsr * 9 + 5], crsr[pcrsr * 9 + 6],
          crsr[pcrsr * 9 + 5] * 4, WL_SHM_FORMAT_ARGB8888);
  wl_pointer_set_cursor(pntr, pntr_serial, crsr_surf,
          crsr[pcrsr * 9 + 7], crsr[pcrsr * 9 + 8]);
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
  unsigned d, c = 0xffaaaa66;
  int i, j, l, m, n, o;
  int k = (bffhght / 2) * bffwdth + (bffwdth / 2);
  double r;
  j = bffwdth * bffhght;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
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
            //Section: show foundry
  if (crsrscale == 1.0 && bffwdth >= 832 && bffhght >= 540) {
    c = 0;
    l = fhght * 6 + 12;
    for (j = 12; j < l; j++) {
      if (j % 6 == 5) {
        c += fwdth;
        continue;
      }
      m = c;
      n = fwdth * 6 + 12;
      for (i = 12; i < n; i++) {
        if (i % 6 == 5) {
          m++;
          continue;
        }
        d = 0xff000000;
        r = 1 - ((f[m] & 0xff000000) >> 24) / 255.0;
        o = ((f[m] & 0xff0000) >> 16) + r *
                ((p[j * bffwdth + i] & 0xff0000) >> 16) + 0.5;
        d += o * 0x10000;
        o = ((f[m] & 0xff00) >> 8) + r *
                ((p[j * bffwdth + i] & 0xff00) >> 8) + 0.5;
        d += o * 0x100;
        o = (f[m] & 0xff) + r * (p[j * bffwdth + i] & 0xff) + 0.5;
        d += o;
        p[j * bffwdth + i] = d;
      }
    }
    cast_cursor(0, 520, 10);
    cast_cursor(1, 690, 150);
    cast_cursor(2, 460, 320);
  }
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, unsigned serial) {
  if (redraw && (surfwidth != bffwdth || surfheight != bffhght)) {
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

void output_geometry(void* data ,wl_output *wl_output,
        int x, int y, int physical_width,
        int physical_height, int subpixel, const char *make,
        const char *model, int transform) {
}

void output_mode(void *data, wl_output *wl_output,
        unsigned flags, int width, int height,
        int refresh) {
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
  printf("Global Object listed as '%d' is removed.\n", name);
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
  int size = hrdwrX * hrdwrY * 4 + 14784;             //<unistd.h>
  ftruncate(fd, size);
  p = (unsigned*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
          fd, 0);                                     //<sys/mman.h>
  pool = wl_shm_create_pool(shm, fd, size);
  close(fd);
                //Section: Prep Cursors
  crsr_surf = wl_compositor_create_surface(comp);
  size -= 14784;
  crsr[0] += size; crsr[9] += size; crsr[18] += size;
  scale_cursors();
  cursors_draw();
                //Section: Show window
  surfwidth = 864; surfheight = 486;
  draw();
  while (wl_display_dispatch(dis) != -1) {
    if (xit) break;
  }
  wl_shm_pool_destroy(pool);
  wl_display_disconnect(dis);
}

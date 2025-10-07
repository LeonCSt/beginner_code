/* leon440.cpp | Window appears, redraws when resized,
     [Q] or [ESC] to close                      [250827]
         or  window manager [key binding] to close focused window
   In the working directory : - 
  wayland-scanner private-code \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-protocol.cpp
   MODIFY xdg-shell-protocol.cpp TO SUIT g++ COMPILER AS FOLLOWS:-
   1) Delete the first 52 lines e.g. all includes and externs
   2] Replace with #include <wayland-util.h>
       and         #include "xdg-shell-client-protocol.h"
   3) Remove all WL_PRIVATE prepends (all 5 of them).

   Then this also : -
  wayland-scanner client-header \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-client-protocol.h

   COMPILE : - 
   g++ xdg-shell-protocol.cpp leon440.cpp -o leon -lwayland-client

   Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./leon    */

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
wl_surface *surf;
xdg_surface *xdgsurf;
xdg_toplevel *xdgtop;
wl_shm_pool *pool;
wl_buffer *buff;
wl_keyboard *kybrd;
unsigned *p; // pixel array for window
bool xit = false, redraw = true;
int hrdwrX, hrdwrY;
int surfwidth, surfheight, wm_capabils;
int bffwdth, bffhght;

void top_config(void *data, xdg_toplevel *xdgtop, int width,
        int height, wl_array *states) {
  surfwidth = width; surfheight = height;
}

void top_close(void *data, xdg_toplevel *xdgtop) {
  xit = true;
}

void top_wm_capabils(void *data, xdg_toplevel *xdgtop,
        wl_array *caps) {
  printf("wm_caps[0] = %d\n", ((unsigned*)caps->data)[0]);
  printf("wm_caps[1] = %d\n", ((unsigned*)caps->data)[1]);
  printf("wm_caps[2] = %d\n", ((unsigned*)caps->data)[2]);
  printf("wm_caps[3] = %d\n", ((unsigned*)caps->data)[3]);
}

xdg_toplevel_listener top_lstnr = {
  .configure = top_config,
  .close = top_close,
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

/* "LINUX KEY _CODES"
 _ESC 1, _1 2, _2 3, _3 4, _4 5, _5 6, _6 7, _7 8, _8 9, _9 10,
 _0 11, _MINUS 12, _EQUAL 13, _BACKSPACE 14, _TAB 15, _Q 16, _W 17,
 _E 18, _R 19, _T 20, _Y 21, _U 22, _I 23, _O 24, _P 25,
 _LEFTBRACE 26, _RIGHTBRACE 27, _ENTER 28, _LEFTCTRL 29, _A 30,
 _S 31, _D 32, _F 33, _G 34, _H 35, _J 36, _K 37, _L 38,
 _SEMICOLON 39, _APOSTROPHE 40, _GRAVE 41, _LEFTSHIFT 42,
 _BACKSLASH 43, _Z 44, _X 45, _C 46, _V 47, _B 48, _N 49, _M 50,
 _COMMA 51, _DOT 52, _SLASH 53, _RIGHTSHIFT 54, _KPASTERISK 55,
 _LEFTALT 56, _SPACE 57, _CAPSLOCK 58, _F1 59, _F2 60, _F3 61,
 _F4 62, _F5 63, _F6 64, _F7 65, _F8 66, _F9 67, _F10 68,
 _NUMLOCK 69, _SCROLLLOCK 70, _KP7 71, _KP8 72, _KP9 73,
 _KPMINUS 74, _KP4 75, _KP5 76, _KP6 77, _KPPLUS 78, _KP1 79,
 _KP2 80, _KP3 81, _KP0 82, _KPDOT 83, _ZENKAKUHANKAKU 85,
 _102ND 86, _F11 87, _F12 88, _RO 89, _KATAKANA 90, _HIRAGANA 91,
 _HENKAN 92, _KATAKANAHIRAGANA 93, _MUHENKAN 94, _KPJPCOMMA 95,
 _KPENTER 96, _RIGHTCTRL 97, _KPSLASH 98, _SYSRQ 99, _RIGHTALT 100,
 _LINEFEED 101, _HOME 102, _UP 103, _PAGEUP 104, _LEFT 105,
 _RIGHT 106, _END 107, _DOWN 108, _PAGEDOWN 109, _INSERT 110,
 _DELETE 111, _MACRO 112, _MUTE 113, _VOLUMEDOWN 114, _VOLUMEUP 115,
 _POWER 116,  _KPEQUAL 117, _KPPLUSMINUS 118, _PAUSE 119, _SCALE 120 */

void kybrd_key(void *data, wl_keyboard *kybrd, unsigned serial,
        unsigned time, unsigned key, unsigned state) {
  if (state == 1) {
    printf("key %d pressed\n", key);
    return;
  }
  if (key == 16 || key == 1) xit = true;
  printf("key %d released\n", key); 
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

void buff_release(void *data, wl_buffer *wl_buffer) {
  printf("buffer released back to the client.\n");
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
  for (int i = 0; i < (bffwdth * bffhght); i++) p[i] = 0x000022;
  double r; 
  int c = 0xaaaa66, j, m, n, o;
  int k = (bffhght / 2) * bffwdth + (bffwdth / 2);
  if (bffwdth <= bffhght) r = bffwdth * 0.45;
  else r = bffhght * 0.45;
  o = 0.7071 * r + 1.5;
  for (int i = 0; i < o; i++) {
    j = cos(asin(i / r)) * r + 0.5;
    n = i * bffwdth; m = j * bffwdth;
    p[k + i + m] = c; p[k + i - m] = c;
    p[k - i + m] = c; p[k - i - m] = c;
    p[k + j + n] = c; p[k + j - n] = c;
    p[k - j + n] = c; p[k - j - n] = c;
  }
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
  printf("another commit: size: %d X %d\n", bffwdth, bffhght);
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

void output_geometry(void* data, wl_output *wl_output,
        int x, int y, int physical_width,
        int physical_height, int subpixel, const char *make,
        const char *model, int transform) {
}

void output_mode(void *data, wl_output *wl_output,
        uint flags, int width, int height,
        int refresh) {
  hrdwrX = width; hrdwrY = height;
  printf("Hardware units: %d X %d   Refresh rate: %d mHz\n",
          width, height, refresh);
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
//    printf("OK! '%s'  '%d'  bound.\n", interface, name);
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
  xdg_toplevel_set_min_size(xdgtop, 864, 486);
  xdg_toplevel_set_title(xdgtop,"Test GUI");
  wl_surface_commit(surf);
  kybrd = wl_seat_get_keyboard(seat);
  wl_keyboard_add_listener(kybrd, &kybrd_lstnr, NULL);
              //Section: Prep Shared memory Buffer
  int fd = syscall(SYS_memfd_create, "shm-leon", 0);  //<sys/syscall.h>
  int size = hrdwrX * hrdwrY * 4;                     //<unistd.h>
  ftruncate(fd, size);
  p = (unsigned*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
          fd, 0);                                     //<sys/mman.h>
  pool = wl_shm_create_pool(shm, fd, size);
  close(fd);
              //Section: Show window
  surfwidth = 864; surfheight = 486;
  draw();
  while (wl_display_dispatch(dis) != -1) {
    if (xit) break;
  }
  wl_shm_pool_destroy(pool);
  wl_display_disconnect(dis);
}

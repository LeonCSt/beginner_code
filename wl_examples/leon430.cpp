/* leon430.cpp | One of my early attempts at Wayland client GUI app.
   Window appears, to be moved or resized,
     then auto exits after a delay.                      [250819]

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
   g++ xdg-shell-protocol.cpp leon430.cpp -o leon -lwayland-client

   Optionally, run with debug > >$ WAYLAND_DEBUG=1 ./leon    */

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <wayland-client.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <math.h>
#include "xdg-shell-client-protocol.h"
using namespace std;

wl_display *dis;
wl_registry *reg;
wl_compositor *comp;
wl_shm *shm;
wl_surface *surf;
xdg_wm_base *xdgwmb;
xdg_surface *xdgsurf;
xdg_toplevel *xdgtop;
wl_buffer *buff;
uint32_t *p; // pixel array for window
int delay = 50, count = 0;
bool xit = false;

void escapement() {
  while (count < delay) {
    this_thread::sleep_for(chrono::milliseconds(100));
    count ++;
  }
  xit = true;
}

void draw() {
  for (int i = 0; i < 307200; i++) p[i] = 0x000033;
  int c = 0xaaaa66, j, k = 153920, m, n;
  for (int i = 0; i < 142; i++) {
    j = cos(asin(i / 200.0)) * 200.0 + 0.5;
    n = i * 640; m = j * 640;
    p[k + i + m] = c; p[k + i - m] = c;
    p[k - i + m] = c; p[k - i - m] = c;
    p[k + j + n] = c; p[k + j - n] = c;
    p[k - j + n] = c; p[k - j - n] = c;
  }
}

void xdgsurf_cnfgr(void *data, xdg_surface *xdgsurf, uint32_t serial) {
  xdg_surface_ack_configure(xdgsurf, serial);
  delay = count + 50;
}

xdg_surface_listener xdgsrf_lstnr = {
  .configure = xdgsurf_cnfgr,
};

void xdgwmb_ping(void *data, xdg_wm_base *xdgwmb, uint32_t serial) {
  xdg_wm_base_pong(xdgwmb, serial);
}

xdg_wm_base_listener wmb_lstnr = {
  .ping = xdgwmb_ping,
};

void buff_release(void *data, wl_buffer *wl_buffer) {
  printf("The buffer has been released back to the client.\n");
}

wl_buffer_listener buff_listnr = {
  .release = buff_release,
};

void registry_global(void *data, wl_registry *regi,
	uint32_t name, const char *interface, uint32_t version) {
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    comp = (wl_compositor*)wl_registry_bind(
            regi, name, &wl_compositor_interface, version);
  }
  else if (strcmp(interface, wl_shm_interface.name) == 0) {
    shm = (wl_shm*)wl_registry_bind(
            regi, name, &wl_shm_interface, version);
  }
  else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    xdgwmb = (xdg_wm_base*)wl_registry_bind(
            regi, name, &xdg_wm_base_interface, version);
//    printf("OK! '%s'  '%d'  bound.\n", interface, name);
  }
}

void registry_global_remove(void *data,
        wl_registry *reg, uint32_t name) {
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
  surf = wl_compositor_create_surface(comp);
  xdg_wm_base_add_listener(xdgwmb, &wmb_lstnr, NULL);
  xdgsurf = xdg_wm_base_get_xdg_surface(xdgwmb, surf);
  xdg_surface_add_listener(xdgsurf, &xdgsrf_lstnr, NULL);
  xdgtop = xdg_surface_get_toplevel(xdgsurf);
  xdg_toplevel_set_title(xdgtop,"Test GUI");
  wl_surface_commit(surf);
              //Section: Prep Shared memory Buffer
  char *path = getenv("XDG_RUNTIME_DIR");
  strcat(path,"/shm-leon");
  int fd = open(path, O_RDWR | O_EXCL | O_CREAT); //<fcntl.h>
  unlink(path);    //<unistd.h>
  int size = 1228800;    // 640 X 480
  ftruncate(fd, size);
  p = (unsigned*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
          fd, 0); //<sys/mman.h>
  wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  buff = wl_shm_pool_create_buffer(pool, 0, 640, 480, 2560,
          WL_SHM_FORMAT_XRGB8888);
  wl_buffer_add_listener(buff, &buff_listnr, NULL);
  wl_shm_pool_destroy(pool);
  close(fd);
              //Section: Show window
  draw();
  wl_surface_attach(surf, buff, 0, 0);
  wl_surface_commit(surf);
  thread t1(escapement); t1.detach();
  while (wl_display_dispatch(dis) != -1) {
    if (xit) break;
  }
  wl_display_disconnect(dis);
}

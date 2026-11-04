/*  leon492.cpp | code that handles the popup dialog.
      [Tab] [Enter] keyboard interact, or use mouse.
      [Q] or [ESC] to close dialog <-- (pointer must be over popup).

    *needs*  leon490.h    | the header file for extern variables
             leon491.cpp  | the main file that calls this popup dialog

    *needs*  xdg-shell-protocol.cpp       | see wl_examplesREADME.txt
             xdg-shell-client-protocol.h  |

    g++ -I/usr/include/freetype2 xdg-shell-protocol.cpp leon491.cpp leon492.cpp -o leon -lwayland-client -lwayland-cursor -lfreetype
                                                       [251113] */

#include "leon490.h"
using namespace std;

/* extern  wl_display *dis;
           wl_compositor *comp;
           wl_shm *shm;
           xdg_wm_base *xdgwmb;
           wl_seat *seat;
           wl_surface *d_surf;
           xdg_surface *xdgsurf;
           wl_cursor_theme *crsr_theme;
           wl_cursor *crsr;
	   FT_Library lbrry;
           FT_Face face;
           int surfW, surfH;
           int txtH
           atomic <int> mX, mY;
           bool setting, popfocus, popup;
           atomic <unsigned> pntr_serial, d_key, d_key_state;
           atomic <unsigned> d_pntr_btn, d_pntr_btn_state;  */

xdg_positioner *d_xdgposi;
xdg_surface *d_xdgsurf;
xdg_popup *d_xdgpop;
wl_shm_pool *d_pool;
wl_buffer *d_buff;
unsigned *d; // pixel array for dialog window
int d_bufW, d_bufH, d_size;
atomic <int> d_btn, btns_x, btns_y, btns_w, btns_h, btns_tb, btns_o;
atomic <bool> d_draw;
mutex d_mutex;


void draw_btns() {
  d_draw =  false;
  int i, j, k, l;
  unsigned fg = 0xff779999, bg = 0xff1d101d, c;
  unsigned char s[] = "OFF\0ON\0\0Exit";
  j = btns_x + 1; k = 0;
  for (i = 0; i < 3; i++) {
    if (i == d_btn) c = 0xff2e222e; else c = bg; 
    fill_rectangle(d, d_bufW, d_bufH, j, btns_y + 1, btns_w - 2,
            btns_h - 2, c);
    if ((i == 0 && !setting) || (i == 1 && setting)) {
      draw_rectangle(d, d_bufW, d_bufH, j + txtH,
              btns_y + txtH * 2, txtH * 2, 2, fg);
    }
    l = j + (btns_w - text_width(&s[k], 4)) / 2.0 + 0.5;
    text_run(d, d_bufW, d_bufH, l, btns_y + btns_o, &s[k], 4, fg, c);
    j += btns_tb;
    k += 4;
  }
}

void refresh_btns() {
  d_draw = false;
  draw_btns();
  wl_surface_attach(d_surf, d_buff, 0, 0);
  wl_surface_damage(d_surf, btns_x + 1, btns_y + 1,
          2 * btns_tb + btns_w - 2, btns_h - 2);
  wl_surface_commit(d_surf);
}

void d_close() {
  std::lock_guard<std::mutex> lock(d_mutex);
  if (d_xdgpop) { xdg_popup_destroy(d_xdgpop); d_xdgpop = NULL; }
  if (d_xdgsurf) { xdg_surface_destroy(d_xdgsurf); d_xdgsurf = NULL; }
  if (d_surf) { wl_surface_destroy(d_surf); d_surf = NULL; }
  if (d_buff) { wl_buffer_destroy(d_buff); d_buff = NULL; }
  if (d_pool) { wl_shm_pool_destroy(d_pool); d_pool = NULL; }
  if (d) { munmap(d, d_size); d = NULL; }
  if (d_xdgposi) { xdg_positioner_destroy(d_xdgposi); d_xdgposi = NULL;}
}

void user_input() {
  if (d_btn == 0) setting = false;
  else if (d_btn == 1) setting = true;
  thread t3(d_close); t3.detach();
  dialog_return();
}

void d_pop_config(void *data, xdg_popup *xdg_popup, int x, int y,
        int width, int height) {
}

void d_pop_done(void *data, xdg_popup *xdgpop) {
  printf("d_pop_done\n");
  d_close();
}

void d_pop_reposi(void *data, xdg_popup *xdgpop, unsigned token) {
}

xdg_popup_listener d_xdgpop_lstnr = {
  .configure = d_pop_config,
  .popup_done = d_pop_done,
  .repositioned = d_pop_reposi,
};

void d_kbd_key() {
        /*    _ESC 1,  _TAB 15,  _Q 16,  _ENTER 28, _SPACE 57   */
  if (d_key_state == 1) {}
  else {
    if (d_key == 16 || d_key == 1) {
      thread t3(d_close); t3.detach();
      dialog_return();
    }
    else if (d_key == 15 && d_draw) {
      d_btn++;
      if (d_btn == 3) d_btn = -1;
      refresh_btns();
      wl_display_flush(dis);
    }
    else if (d_btn > -1 && (d_key == 28 || d_key == 57)) user_input();
  } 
}

void d_pntr_motion() {
  int i = -1, j;
  bool f = false;
  if ((mY > btns_y) && (mY < btns_y + btns_h) && (mX > btns_x)) {
    j =((mX - btns_x) % btns_tb);
    if  (j < btns_w) {
      i = ((mX - btns_x) / btns_tb);
    }
  }
  if ((d_btn == -1) && (i > -1) && (i < 3)) {
    std::lock_guard<std::mutex> lock(d_mutex);
    f = true;
    crsr = wl_cursor_theme_get_cursor(crsr_theme, "hand2");
  }
  else if ((d_btn > -1) && (i < 3) && (d_btn != i)) {
    std::lock_guard<std::mutex> lock(d_mutex);
    f = true;
    crsr = wl_cursor_theme_get_cursor(crsr_theme, "left_ptr");
  }
  if (f && d_draw) {
    set_cursor();
    d_btn = i;
    refresh_btns();
  }
}

void d_pntr_button() {
  if (d_pntr_btn_state == 1) {}
  else {
    if (d_btn > -1) user_input();
  }
}

void d_buff_release(void *data, wl_buffer *wl_buffer) {
  d_draw = true;
}

wl_buffer_listener d_buff_listnr = {
  .release = d_buff_release,
};

void d_draw_pop() {
  d_draw = false;
  int i, j = d_bufW * d_bufH;
  unsigned fg = 0xff779999, bg = 0xff1d101d;
  for (i = 0; i < j; i++) d[i] = bg;
  draw_rectangle(&d[0], d_bufW, d_bufH, 0, 0, d_bufW, d_bufH, fg);
  unsigned char prmpt[] = "Would you like to toggle option 'X'?";
  i = strlen(reinterpret_cast<const char*>(prmpt));
  text_run(&d[0], d_bufW, d_bufH, txtH * 1.5 + 0.5,
          txtH * 1.8 + 0.5, &prmpt[0], i, fg, bg);
  draw_rectangle(&d[0], d_bufW, d_bufH, btns_x, btns_y,
          btns_w, btns_h, fg);
  draw_rectangle(&d[0], d_bufW, d_bufH, btns_x + btns_tb, btns_y,
          btns_w, btns_h, fg);
  draw_rectangle(&d[0], d_bufW, d_bufH, btns_x + 2 * btns_tb, btns_y,
          btns_w, btns_h, fg);
  draw_btns();
  wl_buffer_add_listener(d_buff, &d_buff_listnr, NULL);
  wl_surface_attach(d_surf, d_buff, 0, 0);
  wl_surface_damage(d_surf, 0, 0, d_bufW, d_bufH);
  wl_surface_commit(d_surf);
}

void d_xdgsurf_cnfgr(void *data, xdg_surface *d_xdgsurf,
        unsigned serial) {
  xdg_surface_ack_configure(d_xdgsurf, serial);
}

xdg_surface_listener d_xdgsrf_lstnr = {
  .configure = d_xdgsurf_cnfgr,
};

void maind() {
  std::lock_guard<std::mutex> lock(d_mutex);
  int X, Y, W, H;
  W = 18 * txtH; H = 7 * txtH;
  if (W > surfW - 20 || H > surfH - 20) {
    dialog_return();
    return;
  }
  d_xdgposi = xdg_wm_base_create_positioner(xdgwmb);
  xdg_positioner_set_size(d_xdgposi, W, H);
  X = mX - 9 * txtH; Y = mY + - 3;
  if (X < 10) X = 10;
  else if (X > surfW - W - 10) X = surfW - W - 10;
  if (Y < 10) Y = 10;
  else if (Y > surfH - H - 10) Y = surfH - H - 10; 
  xdg_positioner_set_anchor_rect(d_xdgposi, X, Y, W, H);
  d_surf = wl_compositor_create_surface(comp);
  d_xdgsurf = xdg_wm_base_get_xdg_surface(xdgwmb, d_surf);
  xdg_surface_add_listener(d_xdgsurf, &d_xdgsrf_lstnr, NULL);
  d_xdgpop = xdg_surface_get_popup(d_xdgsurf, xdgsurf, d_xdgposi);
  xdg_popup_add_listener(d_xdgpop, &d_xdgpop_lstnr, NULL);
  wl_surface_commit(d_surf);
                //Section: Prep Shared memory Buffer
  int d_fd = syscall(SYS_memfd_create, "shm-dial", 0); //<sys/syscall.h>
  d_size = W * H * 4;                                  //<unistd.h>
  ftruncate(d_fd, d_size);
  d = (unsigned*) mmap(NULL, d_size, PROT_READ | PROT_WRITE, MAP_SHARED,
          d_fd, 0);                                    //<sys/mman.h>
  d_pool = wl_shm_create_pool(shm, d_fd, d_size);
  close(d_fd);
  d_bufW = W; d_bufH = H;
  d_buff = wl_shm_pool_create_buffer(d_pool, 0, d_bufW, d_bufH,
          d_bufW * 4, WL_SHM_FORMAT_XRGB8888);
                //Section: Draw and Show popup
  d_btn = -1; btns_x = txtH * 2; btns_y = txtH * 3;
  btns_w = txtH * 4; btns_h = txtH * 2.7 + 0.5;
  btns_tb = txtH * 5; btns_o = txtH * 1.7 + 0.5;
  d_draw_pop();
  popup = true;
  wl_display_flush(dis);
}

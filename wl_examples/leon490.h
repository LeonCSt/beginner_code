/*  leon490.h | Header file for leon491.cpp and leon492.cpp
         leon491.cpp handles the main window
         leon492.cpp handles the dialog popup window


                                                       [251113] */
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
#include "xdg-shell-client-protocol.h"

extern wl_display *dis;
extern wl_compositor *comp;
extern wl_shm *shm;
extern xdg_wm_base *xdgwmb;
extern wl_seat *seat;
extern wl_surface *d_surf;
extern xdg_surface *xdgsurf;
extern wl_cursor_theme *crsr_theme;
extern wl_cursor *crsr;
extern FT_Face face;
extern int surfwidth, surfheight;
extern int txthght, mX, mY, d_mx, d_my;
extern bool setting, popfocus, popup;
extern unsigned pntr_serial, d_key, d_key_state;
extern unsigned d_pntr_btn, d_pntr_btn_state;

void maind();

void d_close();

void dialog_return();

void set_cursor();

void d_kbd_key();

void d_pntr_motion();

void d_pntr_button();

int text_width(unsigned char* str, int length);

void text_run(unsigned *m, int bW, int bH, int X, int Y,
        unsigned char *str, int length, unsigned fg, unsigned bg);

void fill_rectangle(unsigned *m, int bW, int bH, int X, int Y,
        int width, int height, unsigned fg);

void draw_rectangle(unsigned *m, int bW, int bH, int X, int Y,
        int width, int height, unsigned fg);

/*  leon740.h | header file for leon741.cpp and leon742.cpp
         leon741.cpp handles the main window
         leon742.cpp handles the popup dialog window  [260115] */



#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <thread>

extern HWND hwnd, dhwnd;
extern HINSTANCE sharedInstance;
extern HFONT hfont;
extern HCURSOR hand;
extern bool setting;
extern int txthght, bufW, bufH;
extern int pX, pY;
extern short mx, my;


void wnDialog();

void dDraw();

void cleanup();

void dialogReturn();


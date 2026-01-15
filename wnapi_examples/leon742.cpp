/*  leon742.cpp | Launch a popup dialog.        [260115]
                  This handles the code for the popup dialog
      Get user's input for a simple On or Off 'setting'.
        [Tab]  and  [Enter]  keyboard interact, or use mouse.  
          [Esc]  or  [Q]  or  Alt F4  or clicking on [Exit] or
            clicking on the parent client area will close it.

      Needs  leon740.h  and  leon741.cpp  residing in working directory

  g++ leon741.cpp leon742.cpp -o leon.exe -lgdi32 -mwindows -municode
*/

#include "leon740.h"
using namespace std;

      /*//Section: extern variables declared in leon740.h
    HWND hwnd, dhwnd;
    HINSTANCE sharedInstance;
    HFONT hfont;
    HCURSOR hand;
    bool setting;
    int txthght, bufW, bufH;
    int pX, pY;
    short mx, my;
        //End Section */

HDC dmhdc;
BITMAPINFO dbmi;
HBITMAP dBitmap;
HBRUSH dbg, dhl, dfg;
RECT drct, dbrct;
unsigned *d;         // pixel array for window
SIZE dsz;
bool dcommit, d_draw = true;
int d_mx, d_my, dX, dY, dW, dH;
int d_btn, btns_x, btns_y, btns_w, btns_h, btns_tb, btns_o;
wchar_t title[] = L"Would you like to toggle option 'X'?";
wchar_t s[] = L"OFF\0ON\0\0Exit";

void cleanup() {
  printf("Cleanup before close\n"); fflush(stdout);
  DeleteObject(dbg); DeleteObject(dhl); DeleteObject(dfg);
  DeleteObject(dBitmap);
  DeleteDC(dmhdc);
  dBitmap = NULL;
  d = NULL;
  dmhdc = NULL;
  dhwnd = NULL;
}

void draw_btns() {
  d_draw =  false;
  HBRUSH hb;
  SIZE bsz;
  int i, j, k, l, n;
  j = btns_x + 1; k = 0;
  for (i = 0; i < 3; i++) {
    if (i == d_btn) hb = dhl; else hb = dbg; 
    dbrct = {j, btns_y + 1, j + btns_w - 2, btns_y + btns_h - 1};
    FillRect(dmhdc, &dbrct, hb);
    if ((i == 0 && !setting) || (i == 1 && setting)) {
      dbrct = {j + int(txthght * 0.85 + 0.5),
              btns_y + int(txthght * 1.7 + 0.5),
              j + int(txthght * 2.5 + 0.5),
              btns_y + int(txthght * 1.79 + 0.5)};
      FillRect(dmhdc, &dbrct, dfg);
    }
    if (i == 0) n = 3; else if (i == 1) n = 2; else n = 4;
    GetTextExtentPoint32(dmhdc, &s[k], n, &bsz);
    l = j + (btns_w - bsz.cx) / 2.0 - 1.5;
    TextOut(dmhdc, l, btns_y + btns_o, &s[k], n); 
    j += btns_tb;
    k += 4;
  }
}

void refresh_btns() {
  d_draw = false;
  draw_btns();
  drct = {btns_x + 1, btns_y + 1, btns_x + 2 * btns_tb + btns_w - 1,
          btns_y + btns_h - 1};
  dcommit = true;
  InvalidateRect(dhwnd, &drct, FALSE);
}

void user_input() {
  /*bool pflag = false;
  if (d_btn == 0) { setting = false; pflag = true; }
  else if (d_btn == 1) { setting = true; pflag = true; }
  else if (d_btn == 2) { PostMessage(dhwnd, WM_CLOSE, 0, 0);  return; }
  if (pflag && d_draw) refresh_btns(); */
  if (d_btn == 0) setting = false;
  else if (d_btn == 1) setting = true;
  PostMessage(dhwnd, WM_CLOSE, 0, 0);
}

void dDraw() {
  d_draw = false;
  int i, j;
              //Section: Direct pixel drawing
  j = dW * dH;
  for (i = 0; i < j; i++) d[i] = 0xff1d101d;
              //Section: GDI drawing
  TextOut(dmhdc, txthght * 0.5, txthght * 0.5, title, wcslen(title));
  j = btns_x;
  for (i = 0; i < 3; i++) {
    dbrct = {j, btns_y, j + btns_w, btns_y + btns_h};
    FrameRect(dmhdc, &dbrct, dfg);
    j += btns_tb;
  } 
  draw_btns();
  drct.left = 0; drct.top = 0;
  drct.right = dW; drct.bottom = dH;
  FrameRect(dmhdc, &drct, dfg);
  dcommit = true;
  InvalidateRect(dhwnd, &drct, FALSE);
}  

LRESULT CALLBACK dialogProc(HWND dhwnd, UINT dMsg, WPARAM wParam,
        LPARAM lParam) {
  switch (dMsg) {
    case WM_KEYUP:
      if (wParam == 9 && d_draw) { // Tab
        d_btn++;
        if (d_btn == 3) d_btn = -1;
        thread t2(refresh_btns); t2.detach();
      }
      else if (wParam == 13 && d_btn > -1) {  // Enter
              thread t2(user_input); t2.detach(); }
      else if (wParam == 27 || wParam == 81) { // Esc  or  [Q]
              PostMessage(dhwnd, WM_CLOSE, 0, 0); }
      return 0; 
    case WM_LBUTTONUP:
        if (d_btn > -1) { thread t2(user_input); t2.detach(); }
      return 0;
    case WM_MOUSEMOVE: {
      d_mx = (short)LOWORD(lParam); d_my = (short)HIWORD(lParam);
      int i = -1, j;
      bool pflag = false;
      if ((d_my > btns_y) && (d_my < btns_y + btns_h)
              && (d_mx > btns_x)) {
        j = ((d_mx - btns_x) % btns_tb);
        if  (j < btns_w) i = ((d_mx - btns_x) / btns_tb);
      }
      if ((d_btn == -1) && (i > -1) && (i < 3)) pflag = true;
      else if ((d_btn > -1) && (i < 3) && (d_btn != i)) pflag = true;
      if (pflag && d_draw) {
        d_btn = i;
        thread t2(refresh_btns); t2.detach();
      }
      return 0;  }
    case WM_PAINT: {
      if (!dcommit) break;
      dcommit = false;
      PAINTSTRUCT dps;
      HDC dhdc = BeginPaint(dhwnd, &dps);
      BitBlt(dhdc, drct.left, drct.top, drct.right - drct.left,
              drct.bottom - drct.top, dmhdc, drct.left, drct.top,
              SRCCOPY);
      EndPaint(dhwnd, &dps);
      d_draw = true;
      return 0; }
    case WM_SETCURSOR:
      if (d_btn == -1) break;
      SetCursor(hand); return 0;
    case WM_SIZE: {
      void *bits = nullptr;
      printf("SIZE:  %d  *  %d\n", LOWORD(lParam), HIWORD(lParam));
      fflush(stdout);
      dW = LOWORD(lParam); dH = HIWORD(lParam);
              //Section: Selecting bitmap into memory DC
      dbmi.bmiHeader.biWidth = dW;
      dbmi.bmiHeader.biHeight = -dH;
      dBitmap = CreateDIBSection(
              dmhdc, &dbmi, DIB_RGB_COLORS, &bits, NULL, 0x0);
      d = (unsigned*)bits;
      SelectObject(dmhdc, dBitmap);
      if (d_draw) { thread t1(dDraw); t1.detach(); }
      return 0; }
    case WM_DESTROY:
      cleanup();
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(dhwnd, dMsg, wParam, lParam);
}

void dinit() {
  hand = LoadCursor(NULL, IDC_HAND);
                //Section: Positioning
  dsz.cx = txthght * 14; dsz.cy = txthght * 6;
  pX = mx - txthght * 7; pY = my + txthght * 0.3;
  if (pX < 10) pX = 10;
  else if (pX > bufW - dsz.cx - 10) pX = bufW - dsz.cx - 10;
  if (pY < 10) pY = 10;
  else if (pY > bufH - dsz.cy - 10) pY = bufH - dsz.cy - 10;
  POINT client = {0, 0};
  ClientToScreen(hwnd, &client);
  dX = pX + client.x; dY = pY + client.y;
                //Section: Prep DC and GDI objects
  dmhdc = CreateCompatibleDC(NULL);
  SetTextColor(dmhdc, RGB(119, 153, 153));
  SetBkColor(dmhdc, RGB(29, 16, 29)); 
  SetBkMode(dmhdc, TRANSPARENT);
  dfg = CreateSolidBrush(RGB(119, 153, 153));
  dbg = CreateSolidBrush(RGB(29, 16, 29));
  dhl = CreateSolidBrush(RGB(46, 34, 46));
  SelectObject(dmhdc,hfont);
  dbmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  dbmi.bmiHeader.biPlanes = 1;
  dbmi.bmiHeader.biBitCount = 32;
  dbmi.bmiHeader.biCompression = BI_RGB;
                //Section: Prep buttons layout
  d_btn = -1; btns_x = txthght; btns_y = txthght * 2.4;
  btns_w = txthght * 3.4; btns_h = txthght * 2.3 + 0.5;
  btns_tb = txthght * 4.25; btns_o = txthght * 0.6 + 0.5;
}

void wnDialog() {
  dinit();
  WNDCLASS dwc = {};
  dwc.lpfnWndProc = dialogProc;
  dwc.hInstance = sharedInstance;
  dwc.lpszClassName = L"Dialog Window";
  dwc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&dwc);
  dhwnd = CreateWindowEx(0, dwc.lpszClassName,
          NULL, WS_POPUP,
          dX, dY, dsz.cx, dsz.cy,
          hwnd, NULL, sharedInstance, NULL);
  if (dhwnd == NULL) {
    printf("Dialog CreateWindow failed\n"); fflush(stdout);
    dialogReturn();
    return;
  }  
  ShowWindow(dhwnd, SW_SHOW);
  if (!IsWindow(dhwnd)) {printf("Is not a Window\n"); fflush(stdout);}
  MSG dmsg = {};
  while (GetMessage(&dmsg, NULL, 0, 0)) {
    TranslateMessage(&dmsg);
    DispatchMessage(&dmsg);
  }
  dialogReturn();
}

/*  leon741.cpp | Launch a popup dialog.        [260115]
            This handles the code for the main parent window.
            Dialog is launched by calling   wnDialog();
              from anywhere in this file.
            Execution resumes in   dialogReturn()  when popup closes.
    Get user's input for a simple On or Off 'setting'.
      [Tab]  or  [Enter]  keyboard interact, or use mouse.  
        [Esc]  or  [Q]  or  Alt F4  or mouse click [X] to close

    Needs  leon740.h  and  leon742.cpp  residing in working directory

  g++ leon741.cpp leon742.cpp -o leon.exe -lgdi32 -mwindows -municode
*/

#include "leon740.h"
using namespace std;

            //Section: defining extern variables declared in leon740.h
HWND hwnd, dhwnd;
HINSTANCE sharedInstance;
HFONT hfont;
HCURSOR hand;
bool setting;
int txthght, bufW, bufH;
int pX, pY;
short mx, my;
            //End Section

HDC mhdc;
HANDLE hMapFile;
BITMAPINFO bmi;
HBRUSH bg, hl, fg;
RECT brct, rct;
unsigned *p;         // pixel array for window
bool commit, redraw = true, btnfocus;
bool mini, popup;
int Hw, Hh, waw, wah, Ww, Wh;
int tp;
wchar_t prompt[] = L"Click to open pop-up dialog";

void refresh_button() {
  redraw = false;
  if (btnfocus) FillRect(mhdc, &brct, hl);
  else FillRect(mhdc, &brct, bg);
  TextOut(mhdc, tp, tp, prompt, wcslen(prompt)); 
  FrameRect(mhdc, &brct, fg);
  rct = brct;
  commit = true;
  InvalidateRect(hwnd, &rct, FALSE);
}

void dialogReturn() {
  popup = false;
  refresh_button();
  printf("Dialog returned\n"); printf("'SETTING'  is  now  ");
  if (setting) printf("TRUE\n"); else printf("FALSE\n");
  fflush(stdout);
  SetFocus(hwnd); 
}

void resizeDraw() {
  redraw = false; 
  bufW = Ww; bufH = Wh;
              //Section: Direct pixel drawing
  int i, j;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
              //Section: Selecting bitmap into memory DC
  bmi.bmiHeader.biWidth = bufW;
  bmi.bmiHeader.biHeight = -bufH;
  HBITMAP hBitmap = CreateDIBSection(
          mhdc, &bmi, DIB_RGB_COLORS, (void**)&p[0], hMapFile, 0x0);
  SelectObject(mhdc, hBitmap);
              //Section: GDI drawing
  TextOut(mhdc, tp, tp, prompt, wcslen(prompt)); 
  FrameRect(mhdc, &brct, fg);
  rct.left = 0; rct.top = 0;
  rct.right = bufW; rct.bottom = bufH;
  mini = false; commit = true;
  InvalidateRect(hwnd, &rct, FALSE); 
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam) {
  switch (uMsg) {
    case WM_GETMINMAXINFO: {
      MINMAXINFO* mmi = (MINMAXINFO*)lParam;
      RECT rect;
      if (popup && !mini) {
        GetWindowRect(hwnd, &rect);
        mmi->ptMinTrackSize.x = rect.right - rect.left;
        mmi->ptMinTrackSize.y = rect.bottom - rect.top;
        mmi->ptMaxTrackSize = mmi->ptMinTrackSize;
      }
      else {
        rect = {0, 0, txthght * 16, txthght * 9};//min client size
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        mmi->ptMinTrackSize.x = rect.right - rect.left;
        mmi->ptMinTrackSize.y = rect.bottom - rect.top;
        mmi->ptMaxTrackSize.x = waw;
        mmi->ptMaxTrackSize.y = wah;
      }
      return 0; }
    case WM_KEYDOWN:
      //printf("KEYDOWN: %d\n", wParam); fflush(stdout);
      if (wParam == 27 || wParam == 81) PostQuitMessage(0); //Esc or [Q]
      return 0;
    case WM_KEYUP:
      if (popup) return 0;
      if (wParam == 9) { // Tab
        btnfocus = !btnfocus;
        if (redraw) { thread t2(refresh_button); t2.detach(); }
      }
      else if (wParam == 13 && btnfocus) { // Enter
        btnfocus = false;
        thread t3(wnDialog); t3.detach();
        popup = true;
      }
      return 0;
    case WM_LBUTTONDOWN:
      if (popup) PostMessage(dhwnd, WM_CLOSE, 0, 0); 
      return 0;
    case WM_LBUTTONUP:
      if (btnfocus) {
        btnfocus = false;
        thread t3(wnDialog); t3.detach();
        popup = true;
      }
      return 0;
    case WM_MOUSEMOVE: {
      if (popup) break;
      mx = (short)LOWORD(lParam); my = (short)HIWORD(lParam);
      bool pflag = false;
      if (btnfocus) {
        if (mx < brct.left || mx >= brct.right ||
                my < brct.top || my >= brct.bottom) {
           btnfocus = false; pflag = true;
        }
      }
      else if (mx >= brct.left && mx < brct.right &&
              my >= brct.top && my < brct.bottom) {
        btnfocus = true; pflag = true;
      }
      if (pflag && redraw) { thread t2(refresh_button); t2.detach(); }
      return 0; }
    case WM_MOVE: {
      if (popup) {
	POINT client = {0, 0};
        ClientToScreen(hwnd, &client);
        SetWindowPos(dhwnd, NULL, pX + client.x, pY + client.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
      }
      return 0;}
    case WM_PAINT: {
      if (!commit) break;
      commit = false;
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint(hwnd, &ps);
      BitBlt(hdc, rct.left, rct.top, rct.right - rct.left,
              rct.bottom - rct.top, mhdc, rct.left, rct.top, SRCCOPY);
      EndPaint(hwnd, &ps);
      redraw = true;
      if (popup) { SetFocus(dhwnd); dDraw(); } 
      return 0; }
    case WM_SETCURSOR:
      if (!btnfocus) break;
      SetCursor(hand); return 0;
    case WM_SIZE: 
      Ww = LOWORD(lParam); Wh = HIWORD(lParam);
      if (Ww < 160 || Wh < 90) { mini = true; return 0; } // minimized
      if (redraw && (bufW != Ww || bufH != Wh || mini)) { 
              thread t1(resizeDraw); t1.detach(); }
      return 0; 
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void init() {
  hand = LoadCursor(NULL, IDC_HAND);
                //Section: Getting output spaces
  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
  waw = workArea.right - workArea.left;
  wah = workArea.bottom - workArea.top;
  DEVMODE dm = { 0 };
  dm.dmSize = sizeof(dm);
  for (int i = 0; EnumDisplaySettings(NULL, i, &dm); ++i) {
    if (dm.dmPelsWidth > Hw
            || (dm.dmPelsWidth == Hw && dm.dmPelsHeight > Hh)) {
      Hw = dm.dmPelsWidth;
      Hh = dm.dmPelsHeight;
    }
  }
                //Section: Prep DC and GDI objects
  mhdc = CreateCompatibleDC(NULL);
  SetTextColor(mhdc, RGB(172, 172, 85));
  SetBkColor(mhdc, RGB(0, 0, 34)); 
  SetBkMode(mhdc, TRANSPARENT);
  fg = CreateSolidBrush(RGB(0, 70, 210));
  bg = CreateSolidBrush(RGB(0, 0, 34));
  hl = CreateSolidBrush(RGB(20, 40, 80));
  txthght = 36;  // <-- this sets the overall scale for everything
  hfont = CreateFont(txthght, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
          TEXT("Times New Roman"));
  SelectObject(mhdc,hfont);
  SIZE sz;
  GetTextExtentPoint32(mhdc, prompt, wcslen(prompt), &sz);
  tp = 2.5 * txthght;
  brct.left = 2 * txthght; brct.top = 2 * txthght;
  brct.right = 3 * txthght + sz.cx; brct.bottom = 4 * txthght;
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
                //Section: Prep memory
  hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
          PAGE_READWRITE, 0, Hw * Hh * 4, L"leon01-shm");
  p = (unsigned*)MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS,
          0, 0, Hw * Hh * 4);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR,
        int nShowCmd) {
  sharedInstance = hInstance;
  init();
  if (waw / txthght < 36 || txthght < 10) {
          printf("txthght out of limit\n"); fflush(stdout); return 1; }
  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"Main Window";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);
  hwnd = CreateWindowEx(0, wc.lpszClassName,
          L"Test GUI - Title Bar Text", WS_OVERLAPPEDWINDOW,
          txthght * 10, txthght * 7, txthght * 24, txthght * 13,
          NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) return 0;
  ShowWindow(hwnd, nShowCmd);
  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (popup) cleanup();
  return 0;
} 

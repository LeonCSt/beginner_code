/*   leon730.cpp | Key/Mouse down driven animation.            [260108]
          [AWSD] to move rectangle.    Mouse to move text.
          Alt F4  or mouse click [X] to close
     g++ leon730.cpp -o leon.exe -lgdi32 -mwindows -municode   */

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <thread>
#include <vector>
using namespace std;

HWND hwnd;
HDC mhdc;
HANDLE hMapFile;
BITMAPINFO bmi;
HFONT hfont;
HBRUSH bg, fg;
RECT trct, rrct, hst_trct, hst_rrct;
SIZE tsz, rsz = {180, 60};
vector<RECT> rct;
unsigned *p;         // pixel array for window
bool commit, redraw = true;
bool xit, keydwn, msdwn, tdrw, rdrw;
bool ky[4]; // AWSD key flags
int Hw, Hh, waw, wah, Ww, Wh, bufW, bufH;
int tX = 50, tY = 50, rX = 200, rY = 200;
short mx, my;

void draw_animat() {
  if (tdrw && (tX != hst_trct.left || tY != hst_trct.top )) {
    FillRect(mhdc, &hst_trct, bg);
    rct.push_back(hst_trct);
    hst_trct = trct;
  }
  if (rdrw && (rX != hst_rrct.left || rY != hst_rrct.top )) {
    FillRect(mhdc, &hst_rrct, bg);
    rct.push_back(hst_rrct);
    hst_rrct = rrct;
  }
  if (rdrw) {
    FillRect(mhdc, &hst_rrct, fg);
    rct.push_back(rrct);
  } 
  if (tdrw) {
    TextOut(mhdc, tX, tY, L"Some Text", 9); 
    rct.push_back(trct);
  }
  commit = true;
      for (int i = 0; i < rct.size(); i++) {
              InvalidateRect(hwnd, &rct[i], FALSE); }
}

void resizeDraw() {
  redraw = false; 
  bufW = Ww; bufH = Wh;
              //Section: Direct pixel drawing
  int i, j;
  unsigned c = 0xffaaaa66;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
              //Section: Selecting bitmap into memory DC
  bmi.bmiHeader.biWidth = bufW;
  bmi.bmiHeader.biHeight = -bufH;
  HBITMAP hBitmap = CreateDIBSection(
          mhdc, &bmi, DIB_RGB_COLORS, (void**)&p[0], hMapFile, 0x0);
  SelectObject(mhdc, hBitmap);
              //Section: GDI drawing
  if (rX > bufW - rsz.cx) rX = bufW - rsz.cx;//elements maybe offscreen
  if (rY > bufH - rsz.cy) rY = bufH - rsz.cy;
  if (tX > bufW - tsz.cx) tX = bufW - tsz.cx;
  if (tY > bufH - tsz.cy) tY = bufH - tsz.cy;
  trct.left = tX; trct.top = tY; //text rectangle
  trct.right = tX + tsz.cx; trct.bottom = tY + tsz.cy;
  hst_trct = trct;
  rrct.left = rX; rrct.top = rY; //solid rectangle
  rrct.right = rX + rsz.cx; rrct.bottom = rY + rsz.cy;
  hst_rrct = rrct;
  FillRect(mhdc, &rrct, fg);
  TextOut(mhdc, tX, tY, L"Some Text", 9); 
  rct.resize(1);
  rct[0].left = 0; rct[0].top = 0;
  rct[0].right = bufW; rct[0].bottom = bufH;
  commit = true;
  InvalidateRect(hwnd, NULL, FALSE); 
}

void animate() {
  while (!xit) {
    if ((keydwn || msdwn) && redraw) {
      tdrw = false; rdrw = false;
      if (keydwn) {
        if (ky[0] && rX < bufW - rsz.cx - 4) {rX += 5; rdrw = true;}
        if (ky[1] && rX > 4) { rX -= 5; rdrw = true; }
        if (ky[2] && rY < bufH - rsz.cy - 4) {rY += 5; rdrw = true;}
        if (ky[3] && rY > 4) { rY -= 5; rdrw = true; }
        if  (rdrw) {
          redraw = false;
          rrct.left = rX; rrct.top = rY; //solid rectangle
          rrct.right = rX + rsz.cx; rrct.bottom = rY + rsz.cy;
        }
      }
      if (msdwn) {
        int x, y;
        x = mx - 0.5 * tsz.cx; y = my - 0.5 * tsz.cy;
        if (x < 0) x = 0; if (y < 0) y = 0;
        if (x > bufW - tsz.cx) x = bufW - tsz.cx;
        if (y > bufH - tsz.cy) y = bufH - tsz.cy;
        if (x != tX || y != tY) {
          tdrw = true; redraw = false;
          tX = x; tY = y;
          trct.left = tX; trct.top = tY; //text rectangle
          trct.right = tX + tsz.cx; trct.bottom = tY + tsz.cy;
        }
      }
      if (rdrw && !tdrw) { //hit testing rectangles
        if ((hst_rrct.left < trct.right && hst_rrct.right > trct.left
                && hst_rrct.top < trct.bottom
                && hst_rrct.bottom > trct.top)
                || (rrct.left < trct.right && rrct.right > trct.left
                && rrct.top < trct.bottom
                && rrct.bottom > trct.top)) {
          tdrw = true;
        }
      }
      if (!rdrw && tdrw) {
        if ((hst_trct.left < rrct.right && hst_trct.right > rrct.left
                && hst_trct.top < rrct.bottom
                && hst_trct.bottom > rrct.top )
                || (trct.left < rrct.right && trct.right > rrct.left
                && trct.top < rrct.bottom
                && trct.bottom > rrct.top )) {
          rdrw = true;
        }
      }
      if (tdrw || rdrw) { thread t3(draw_animat); t3.detach(); }
    }   
    this_thread::sleep_for(chrono::milliseconds(30));
  }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam) {
  switch (uMsg) {
    case WM_GETMINMAXINFO: {
      RECT rect = {0, 0, 640, 360}; // define minimum client size here
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      MINMAXINFO* mmi = (MINMAXINFO*)lParam;
      mmi->ptMinTrackSize.x = rect.right - rect.left;
      mmi->ptMinTrackSize.y = rect.bottom - rect.top;
      mmi->ptMaxTrackSize.x = waw;
      mmi->ptMaxTrackSize.y = wah;
      return 0; }
    case WM_KEYDOWN:       // _A 65, _W 87, _S 83, _D 68
        //printf("KEYDOWN: %d\n", wParam); fflush(stdout);
      if (wParam == 68 || wParam == 65 || wParam == 83
                       || wParam == 87) {
        if (wParam == 68) ky[0] = true;
        else if (wParam == 65) ky[1] = true;
        else if (wParam == 83) ky[2] = true;
        else ky[3] = true;
        keydwn = true;
      }
      return 0;
    case WM_KEYUP:
        //printf("KEYUP: %d\n", wParam); fflush(stdout);
      if (wParam == 68 || wParam == 65 || wParam == 83
                       || wParam == 87) {
        if (wParam == 68) ky[0] = false;
        else if (wParam == 65) ky[1] = false;
        else if (wParam == 83) ky[2] = false;
        else ky[3] = false;
        if (keydwn && !ky[0] && !ky[1] && !ky[2] && !ky[3])
                keydwn = false;
      }
      return 0;
    case WM_LBUTTONDOWN:
      msdwn = true; return 0;
    case WM_LBUTTONUP:
      msdwn = false; return 0;
    case WM_MOUSEMOVE:
      mx = (short)LOWORD(lParam);
      my = (short)HIWORD(lParam);
      return 0;
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      if (!commit) break;
      hdc = BeginPaint(hwnd, &ps);
      for (int i = 0; i < rct.size(); i++) {
        BitBlt(hdc, rct[i].left, rct[i].top,
                rct[i].right - rct[i].left, rct[i].bottom - rct[i].top,
                mhdc, rct[i].left, rct[i].top, SRCCOPY);
      }
      EndPaint(hwnd, &ps);
      rct.clear();
      commit = false; redraw = true;
      return 0; }
    case WM_SIZE: 
      Ww = LOWORD(lParam); Wh = HIWORD(lParam);
      if (Ww < 640 || Wh < 360)  break; //window minimized
      if (redraw && (bufW != Ww || bufH != Wh)) { 
              thread t2(resizeDraw); t2.detach(); }
      return 0; 
    case WM_DESTROY:
      xit = true;
      this_thread::sleep_for(chrono::milliseconds(30));
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void init() {
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
  hfont = CreateFont(30, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
          TEXT("Times New Roman"));
  SelectObject(mhdc,hfont);
  GetTextExtentPoint32(mhdc, L"Some Text", 9, &tsz);
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
                //Section: Prep memory
  //printf("Setting up shared memory\n"); fflush(stdout);
  hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
          PAGE_READWRITE, 0, Hw * Hh * 4, L"leon01-shm");
  p = (unsigned*)MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS,
          0, 0, Hw * Hh * 4);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR,
        int nShowCmd) {
  init();
  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"Main Window";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);
  hwnd = CreateWindowEx(0, wc.lpszClassName,
          L"Test GUI - Title Bar Text", WS_OVERLAPPEDWINDOW,
          480, 270, 960, 540, NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) return 0;
  ShowWindow(hwnd, nShowCmd);
  thread t1(animate); t1.detach();
  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
} 



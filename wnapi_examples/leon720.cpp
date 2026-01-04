/*   leon720.cpp | UTF16 text rendering.               [260104]
          Alt F4  or mouse click [X] to close
     g++ leon720.cpp -o leon.exe -lgdi32 -mwindows -municode   */

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <thread>
using namespace std;

HWND hwnd;
HDC mhdc;
HANDLE hMapFile;
BITMAPINFO bmi;
HFONT hfont;
unsigned *p;         // pixel array for window
bool commit, redraw = true;
int Hw, Hh, waw, wah, Ww, Wh, bufW, bufH;
wchar_t doc[] = L"Simple rendering. Assuming 'flat' background. Does not offer texture or gradient foreground.\nDoes not 'wrap' long text to the next line. DrawText() does honor newline '\\n'.\nUtf16 encoding L♡VE, j☺y, Pe☮ce.\nRegards ←\0";

void resizeDraw() {
  redraw = false;
  RECT rct, txtRct; 
  bufW = Ww; bufH = Wh;
              //Section: Direct pixel drawing
  int i, j, m, n, o;
  unsigned c = 0xffaaaa66;
  j = bufW * bufH;
  for (i = 0; i < j; i++) p[i] = 0xff000022;
  double r; 
  int k = (bufH / 2) * bufW + (bufW / 2);
  if (bufW <= bufH) r = bufW * 0.45;
  else r = bufH * 0.45;
  o = 0.7071 * r + 1.5;
  for (i = 0; i < o; i++) {
    j = cos(asin(i / r)) * r + 0.5;
    n = i * bufW; m = j * bufW;
    p[k + i + m] = c; p[k + i - m] = c;
    p[k - i + m] = c; p[k - i - m] = c;
    p[k + j + n] = c; p[k + j - n] = c;
    p[k - j + n] = c; p[k - j - n] = c;
  }
              //Section: Selecting bitmap into memory DC
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = bufW;
  bmi.bmiHeader.biHeight = -bufH;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  HBITMAP hBitmap = CreateDIBSection(
          mhdc, &bmi, DIB_RGB_COLORS, (void**)&p[0], hMapFile, 0x0);
  SelectObject(mhdc, hBitmap);
              //Section: GDI drawing
  SelectObject(mhdc,hfont);
  txtRct = {50, 50, bufW - 50, bufH - 50};
  SetTextColor(mhdc, RGB(172, 172, 85));
  SetBkColor(mhdc, RGB(0, 0, 34)); 
  SetBkMode(mhdc, OPAQUE); // can be TRANSPARENT but probably slower.?
  i = lstrlenW(doc);
  //printf("Length of doc[] = %d\n", i); fflush(stdout);
  DrawText(mhdc, &doc[0], i, &txtRct, DT_LEFT); // DT_NOCLIP  is faster
  /*  Alternative to DrawText(): TextOut() - this ignores newlines etc
         and only needs start point. TextOut() is probably faster.? */
  //TextOut(mhdc, 50, 50, &doc[0], i); 
  rct = {bufW - 200 ,bufH - 200, bufW - 100, bufH - 100};
  HBRUSH hBrush = CreateSolidBrush(RGB(0, 70, 210));
  FillRect(mhdc, &rct, hBrush);
  DeleteObject(hBrush);
  commit = true;
  InvalidateRect(hwnd, NULL, FALSE); 
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam) {
  switch (uMsg) {
    case WM_GETMINMAXINFO: {
      RECT rect = {0, 0, 640, 360};  // define minimum client size here
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      MINMAXINFO* mmi = (MINMAXINFO*)lParam;
      mmi->ptMinTrackSize.x = rect.right - rect.left;
      mmi->ptMinTrackSize.y = rect.bottom - rect.top;
      mmi->ptMaxTrackSize.x = waw;
      mmi->ptMaxTrackSize.y = wah;
      return 0; }
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      if (!commit) break;
      hdc = BeginPaint(hwnd, &ps);
      BitBlt(hdc, 0, 0, bufW, bufH, mhdc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &ps);
      commit = false; redraw = true;
      return 0; }
    case WM_SIZE: 
      Ww = LOWORD(lParam); Wh = HIWORD(lParam);
      if (Ww < 640 || Wh < 360)  break; //window minimized
      if (redraw && (bufW != Ww || bufH != Wh)) { 
              thread t1(resizeDraw); t1.detach(); }
      return 0; 
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void init() {
  mhdc = CreateCompatibleDC(NULL);
              //Section: Getting output spaces
  RECT workArea;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
  waw = workArea.right - workArea.left;
  wah = workArea.bottom - workArea.top;
  printf("Working area (virtual)  width %d   height %d\n", waw, wah);
  fflush(stdout);
  DEVMODE dm = { 0 };
  dm.dmSize = sizeof(dm);
  for (int i = 0; EnumDisplaySettings(NULL, i, &dm); ++i) {
    if (dm.dmPelsWidth > Hw ||
        (dm.dmPelsWidth == Hw && dm.dmPelsHeight > Hh)) {
      Hw = dm.dmPelsWidth;
      Hh = dm.dmPelsHeight;
    }
  }
  printf("hardware width %d   height %d\n", Hw, Hh); fflush(stdout);
                //Section: Prep Font
  hfont = CreateFont(30, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
          CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Times New Roman"));
                //Section: Prep memory
  printf("Setting up shared memory\n"); fflush(stdout);
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
  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
} 

/* Convert your utf8 encoded string to utf16 encoding for winapi.

std::wstring utf8_to_utf16(const std::string& utf8) {
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1,
          nullptr, 0);
  std::wstring utf16(len, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], len);
  return utf16;
}

*/

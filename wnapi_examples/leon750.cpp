/*   leon750.cpp | Using the windows clipboard.            [260116]
      [Ctrl] [C]  to cut      [Ctrl] [V]  to paste
      hold[Ctrl] [C]then[V]  to copy
        Textbox starts out empty so you have to copy
	from somewhere else then paste in to get started.
        Example -- copy --> "Utf16 encoding L♡VE, j☺y, Pe☮ce."
	(In gvim it is [Shift] ['][=]  then  [Y] to copy to clipboard.)
      Use mouse to highlight or place caret in text.
        (Not implemented: Keyboard keys to highlight or move caret.)
         [Q] or [ESC] or  Alt F4  or mouse click [X] to close

     g++ leon750.cpp -o leon.exe -lgdi32 -mwindows -municode   */

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
HCURSOR Ibeam;
HFONT hfont;
HBRUSH fg, hl, tb;
RECT rct;
unsigned *p; // pixel array for window
vector<wchar_t> doc = {0};
vector<unsigned> ls = {0}; // ls[]  each lines start pos within doc[]
vector<unsigned short> ll = {0}; //  ll[]   pixel length of each line
bool commit, redraw = true;
int Hw, Hh, waw, wah, Ww, Wh, bufW, bufH;
int txthght;
short mx, my;
int doclength, caret, clp_size, nlines, enddoc;
int txtX, txtY, txtW, txtH, leftM, tabW, lineH, ln_o;
int caretx, carety, histcaret, histcaretx, histcarety;
bool txtbxfocus, txtbxactive, highlight, cursorcaret;
bool blinkeractive, mousedown, blinker;
bool mini, lftctl, refresh, resumeblinker;

void text_run(int X, int Y, int pos, int end) {
  SIZE sz;
  int histpos = pos;
  while (pos < end) {
    if (doc[pos] == 9) {
      X += tabW - (X - leftM) % tabW;
      histpos++;
    }
    else {
      if (doc[pos + 1] == 9 || pos == end - 1) {
	GetTextExtentPoint32(
                mhdc, &doc[histpos], pos - histpos + 1, &sz);
        TextOut(mhdc, X, Y, &doc[histpos], pos - histpos + 1);
        //ExtTextOut(mhdc, X, Y, ETO_IGNORELANGUAGE, nullptr,
                //&doc[histpos], pos - histpos + 1, nullptr);
        X += sz.cx; histpos = pos + 1;
      }
    }
    pos++;
  }
}

void draw_textbox() {
  redraw = false;
  RECT trct;
  int i, j, k, t;
  trct = {txtX, txtY, txtX + txtW, txtY + txtH};
  if (txtbxfocus || txtbxactive) FillRect(mhdc, &trct, hl);
  else FillRect(mhdc, &trct, tb);
  if (doclength > 0) {
    j = ln_o + txtY; t = txtY;
    for (i = 0; i < nlines; i++) {
      if (i == 12) break;
      if (!highlight || ls[i + 1] <= histcaret || ls[i] >= caret) {
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        if (doc[ls[i + 1] - 2] == 13) k--;
        SetTextColor(mhdc, RGB(170, 170, 102));
        text_run(leftM, j, ls[i], ls[i] + k);
      }
      else if (histcaret <= ls[i] && caret >= ls[i + 1]) {
        trct = {leftM, t, leftM + ll[i], t + lineH};
        FillRect(mhdc, &trct, fg);
        k = ls[i + 1] - ls[i];
        if (doc[ls[i + 1] - 1] == 10) k--;
        if (doc[ls[i + 1] - 2] == 13) k--;
        SetTextColor(mhdc, RGB(16, 16, 49)); 
        text_run(leftM, j, ls[i], ls[i] + k);
      }
      else if (histcaret <= ls[i] && caret < ls[i + 1]) {
        trct = {leftM, t, caretx, t + lineH};
        FillRect(mhdc, &trct, fg);
        k = caret - ls[i];
        SetTextColor(mhdc, RGB(16, 16, 49)); 
        text_run(leftM, j, ls[i], ls[i] + k);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        if (doc[ls[i + 1] - 2] == 13) k--;
        SetTextColor(mhdc, RGB(170, 170, 102));
        text_run(caretx, j, caret, caret + k);
      }
      else if (histcaret > ls[i] && caret >= ls[i + 1]) {
        k = histcaret - ls[i];
        SetTextColor(mhdc, RGB(170, 170, 102));
        text_run(leftM, j, ls[i], ls[i] + k);
        trct = {histcaretx, t, leftM + ll[i], t + lineH};
        FillRect(mhdc, &trct, fg);
        k = ls[i + 1] - histcaret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        if (doc[ls[i + 1] - 2] == 13) k--;
        SetTextColor(mhdc, RGB(16, 16, 49)); 
        text_run(histcaretx, j, histcaret, histcaret + k);
      }
      else {
        k = histcaret - ls[i];
        SetTextColor(mhdc, RGB(170, 170, 102));
        text_run(leftM, j, ls[i], ls[i] + k);
        k = ls[i + 1] - caret;
        if (doc[ls[i + 1] - 1] == 10) k--;
        if (doc[ls[i + 1] - 2] == 13) k--;
        text_run(caretx, j, caret, caret + k);
        trct = {histcaretx, t, caretx, t + lineH};
        FillRect(mhdc, &trct, fg);
        k = caret - histcaret;
        SetTextColor(mhdc, RGB(16, 16, 49)); 
        text_run(histcaretx, j, histcaret, histcaret + k);
      }
      j += lineH; t += lineH;
    }
  } 
  if (refresh) {
    rct = {txtX, txtY, txtX + txtW, txtY + txtH};
    commit = true;
    InvalidateRect(hwnd, &rct, FALSE); 
  }
}

void castlines() {
  if (nlines != 0) {
    ls.erase(ls.begin(), ls.begin() + nlines);
    ll.erase(ll.begin(), ll.begin() + nlines);
    nlines = 0;
  }
  ABC abc;
  int pos = 0, tally = 0, space, spcpos, histpos = 0, advance;
  ls[0] = doclength;
  while (pos < doclength) {
    if (pos == caret) {
      caretx = tally + leftM;
      carety = nlines * lineH + txtY;
    }
    if (doc[pos] == 10) {
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      histpos = pos + 1;
      tally = 0; space = 0;
      nlines ++;
      pos ++;
      continue;
    }
    if (doc[pos]== 9) {
      tally += tabW - tally % tabW;
      space = tally;
      spcpos = pos;
    }
    else if (doc[pos] < 32 || (doc[pos] > 127 && doc[pos] < 161)) {
      pos ++; continue;
    }
    else {
      if (GetCharABCWidths(mhdc, doc[pos], doc[pos], &abc))
              advance = abc.abcA + abc.abcB + abc.abcC;
      else advance = 0;
      tally += advance;
      if (doc[pos] == 32) {
        space = tally;
        spcpos = pos;
      }
    }
    if (tally > 23.4 * txthght) {
      ls.insert(ls.begin() + nlines, histpos);
      if (space == 0) {
        ll.insert(ll.begin() + nlines, tally);
        histpos = pos + 1;
      }
      else {
        ll.insert(ll.begin() + nlines, space);
        histpos = spcpos + 1;
        pos = spcpos;
      }
      tally = 0; space = 0;
      nlines ++;
    }
    else if (pos == doclength - 1) {
      if (caret == doclength) {
        caretx = tally + leftM;
        carety = nlines * lineH + txtY;
      }
      ls.insert(ls.begin() + nlines, histpos);
      ll.insert(ll.begin() + nlines, tally);
      nlines ++;
    }
    pos++;
  } 
  if (nlines > 12) enddoc = txtY + 12 * lineH;
  else enddoc = txtY + nlines * lineH;
  if (carety > txtY + 11 * lineH) {
    caret = 0;
    carety = txtY;
    caretx = leftM;
  } 
  if (redraw) {refresh = true; draw_textbox(); }
} 

void findcrsrpos() {
  ABC abc;
  int pos, tally = leftM, advance;
  if (my > txtY + nlines * lineH) {
    caret = doclength;
    caretx = ll[nlines - 1] + leftM;
    carety = (nlines - 1) * lineH + txtY;
  }
  else {
    int j = (my - txtY) / lineH;
    carety = j * lineH + txtY;
    if (mx - leftM > ll[j]) {
      caret = ls[j + 1];
      caretx = ll[j] + leftM;
    }
    else if (mx < leftM) {
      caret = ls[j];
      caretx = leftM;
    }
    else {
      pos = ls[j];
      while (pos < ls[j + 1]) {
        caretx = tally;
        if (doc[pos] == 9) tally += tabW - (tally - leftM) % tabW;
        else if (doc[pos] < 32 || (doc[pos] > 127 && doc[pos] < 161)) {
          pos ++; continue;
        }
        else {
    	  if (GetCharABCWidths(mhdc, doc[pos], doc[pos], &abc)) {
            advance = abc.abcA + abc.abcB + abc.abcC;
          } else advance = 0;
          tally += advance;
        }
        if (tally > mx) break;
        pos ++;
      }
      caret = pos;
    }
  }
}

void caretblinker() {
  blinker = true;
  int i = 0, j, k, x, y;
  unsigned h[lineH];
  x = caretx; y = carety; j = y * bufW + x;
  rct = {x, y, x + 1, y + lineH};
  for (k = 0; k < lineH; k++) { h[k] = p[j]; j += bufW; }
  while (blinkeractive) {
    if (i % 16 == 0 && redraw) {
      redraw = false; j = y * bufW + x;
      for (k = 0; k < lineH; k++) {
        if (blinker) p[j] = 0xffaaaa66; else p[j] = h[k];
	j += bufW;	  
      }
      blinker = !blinker;
      commit = true;
      InvalidateRect(hwnd, &rct, FALSE); 
    }
    this_thread::sleep_for(chrono::milliseconds(23));
    i++;
  }
}

void receive_clpbrd() {
  blinkeractive = false;
  this_thread::sleep_for(chrono::milliseconds(35));
  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
          printf("CF_UNICODETEXT available\n"); fflush(stdout); }
  else if (IsClipboardFormatAvailable(CF_TEXT)) {
          printf("CF_TEXT available\n"); fflush(stdout); }
  else { printf("NO TEXT available\n"); fflush(stdout); return; }
  int i, j;
  OpenClipboard(nullptr);
  HANDLE hData = GetClipboardData(CF_UNICODETEXT);
  wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
  clp_size = lstrlenW(pszText);
  doc.resize(doclength + clp_size + 1);
  for (i = doclength; i >= caret; i--) doc[i + clp_size] = doc[i];
  j = caret;
  for (i = 0; i < clp_size; i++) { doc[j] = pszText[i]; j++; }
  caret += clp_size; doclength += clp_size;
  GlobalUnlock(hData);
  CloseClipboard();
  castlines();
  blinkeractive = true; resumeblinker = true;
}

void send_to_clpbrd() {
  blinkeractive = false;
  this_thread::sleep_for(chrono::milliseconds(35));
  OpenClipboard(nullptr);
  if (!OpenClipboard(nullptr)) return;
  EmptyClipboard();
  int i, j, k;
  j = histcaret;
  k = caret - histcaret;
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (k + 1) * 2);
  if (hMem == nullptr) { CloseClipboard(); return; }
  wchar_t *cData = static_cast<wchar_t*>(GlobalLock(hMem));
  if (cData == nullptr) { GlobalFree(hMem); CloseClipboard(); return; }
  for (i = 0; i < k; i++) {cData[i] = doc[j]; j++; }
  cData[k] = 0;
  GlobalUnlock(hMem);
  HANDLE hResult = SetClipboardData(CF_UNICODETEXT, hMem);
  if (hResult == nullptr) GlobalFree(hMem);
  CloseClipboard();
  printf("Send to Clipboard complete.\n"); fflush(stdout);
  for (i = caret; i <= doclength; i++) doc[i - k] = doc[i];
  doclength -= k; caret -= k;
  doc.resize(doclength + 1);
  highlight = false;
  castlines();
  if (doclength != 0) { blinkeractive = true; resumeblinker = true; }
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
  refresh = false; draw_textbox();
  RECT frct = {txtX - 1, txtY - 1, txtX + txtW + 1, txtY + txtH + 1}; 
  FrameRect(mhdc, &frct, fg);
  rct = {0, 0, bufW, bufH};
  mini = false; commit = true;
  InvalidateRect(hwnd, &rct, FALSE); 
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam) {
  switch (uMsg) {
    case WM_GETMINMAXINFO: {
      RECT rect = {0, 0, txthght * 29, txthght * 18}; //min client size
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      MINMAXINFO* mmi = (MINMAXINFO*)lParam;
      mmi->ptMinTrackSize.x = rect.right - rect.left;
      mmi->ptMinTrackSize.y = rect.bottom - rect.top;
      mmi->ptMaxTrackSize.x = waw;
      mmi->ptMaxTrackSize.y = wah;
      return 0; }
    case WM_KEYDOWN:  //_ESC 27,  _Q 81,  _C 67,  _V 86,  _LEFTCTRL 17
        //printf("KEYDOWN: %d\n", wParam); fflush(stdout);
      if (wParam == 17) lftctl = true;
      return 0;
    case WM_KEYUP:
      if (wParam == 17) lftctl = false;
      else if (wParam == 27 || wParam == 81)
              PostMessage(hwnd, WM_CLOSE, 0, 0);
      else if (lftctl && wParam == 86 && txtbxfocus
              && !highlight) receive_clpbrd();
      else if (lftctl && wParam == 67 && highlight) send_to_clpbrd();
      return 0;
    case WM_LBUTTONDOWN:
      highlight = false;
      if (txtbxfocus) { 
        if (doclength != 0) {
          findcrsrpos();
          histcaret = caret;
          histcaretx = caretx; histcarety = carety;
          mousedown = true;
          txtbxactive = true;
          blinkeractive = false;
        }
      }
      else {
        txtbxactive = false;
        blinkeractive = false;
      }
      if (redraw) {refresh = true;
              thread t3(draw_textbox); t3.detach(); }
      return 0;
    case WM_LBUTTONUP: {
      if (txtbxfocus && doclength != 0) {
        mousedown = false;
        blinkeractive = true;
        thread t1(caretblinker); t1.detach();
      }
      return 0; }
    case WM_MOUSEMOVE:
      mx = (short)LOWORD(lParam); my = (short)HIWORD(lParam);
                //Section: mouse over textbox
      if (mx >= txtX && mx < txtX + txtW && my >= txtY
              && my < txtY + txtH && !txtbxfocus) {
        txtbxfocus = true;
        if (!txtbxactive && redraw) {
                refresh = true; thread t3(draw_textbox); t3.detach(); }
      }
      else if ((mx <= txtX || mx > txtX + txtW || my <= txtY
              || my > txtY + txtH) && txtbxfocus) {
        txtbxfocus = false;
        if (!txtbxactive && redraw) {
                refresh = true; thread t3(draw_textbox); t3.detach(); }
      }
                //section:  mouse over text
      int endline;
      if (doclength != 0) {
        if (my < txtY || my > enddoc || nlines == 0) endline = leftM;
        else endline = leftM + ll[(my - txtY) / lineH];
        if (my > txtY && my < enddoc && mx > leftM
                && mx < endline && !cursorcaret) {
          cursorcaret = true;
        }
        else if ((my < txtY || my > enddoc || mx < leftM
                || mx > endline) && cursorcaret) {
          cursorcaret = false;
        }
      }
                //section: mouse drag to highlight text
      if (mousedown && my > txtY && my < enddoc && redraw) {
        findcrsrpos();
        if (caret > histcaret) highlight = true;
        else highlight = false;	
        if (redraw) { refresh = true;
                thread t3(draw_textbox); t3.detach(); }
      }  
      return 0;
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      if (!commit) break;
      commit = false;
      hdc = BeginPaint(hwnd, &ps);
      BitBlt(hdc, rct.left, rct.top,
              rct.right - rct.left, rct.bottom - rct.top,
              mhdc, rct.left, rct.top, SRCCOPY);
      EndPaint(hwnd, &ps);
      redraw = true;
      if (resumeblinker) { resumeblinker = false;
              thread t1(caretblinker); t1.detach(); }
      return 0; }
    case WM_SETCURSOR:
      if (!cursorcaret) break;
      SetCursor(Ibeam); return 0;
    case WM_SIZE: 
      Ww = LOWORD(lParam); Wh = HIWORD(lParam);
      if (Ww < 290 || Wh < 180) { mini = true; return 0; } //minimized
      if (redraw && (bufW != Ww || bufH != Wh || mini)) { 
              thread t2(resizeDraw); t2.detach(); }
      return 0; 
    case WM_DESTROY:
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
  Ibeam = LoadCursor(NULL, IDC_IBEAM);
                //Section: Prep DC and GDI objects
  mhdc = CreateCompatibleDC(NULL);
  SetBkMode(mhdc, TRANSPARENT);
  fg = CreateSolidBrush(RGB(170, 170, 102));
  tb = CreateSolidBrush(RGB(9, 9, 42));
  hl = CreateSolidBrush(RGB(16, 16, 49));
  txthght = 36; // <- this sets the overall scale of everything
  hfont = CreateFont(txthght, 0, 0, 0, FW_DONTCARE, FALSE, FALSE,
          FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
          TEXT("Times New Roman"));
  SelectObject(mhdc,hfont);
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  txtX = txthght * 2; txtY = txthght * 2; txtW = txthght * 25;
  ABC abc;
  GetCharABCWidths(mhdc, '0', '0', &abc);
  tabW = 8 * (abc.abcA + abc.abcB + abc.abcC);
  lineH = 1.2 * txthght + 0.5; txtH = 12 * lineH;
  leftM = txthght * 2.7 + 0.5; ln_o = 0.05 * txthght + 0.5;
                //Section: Prep memory
  hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
          PAGE_READWRITE, 0, Hw * Hh * 4, L"leon01-shm");
  p = (unsigned*)MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS,
          0, 0, Hw * Hh * 4);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR,
        int nShowCmd) {
  init();
  if (waw / txthght < 34 || wah / txthght < 23 || txthght < 10) {
          printf("txthght out of limit\n"); fflush(stdout); return 1; }
  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"Main Window";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);
  hwnd = CreateWindowEx(0, wc.lpszClassName,
          L"Test GUI - Title Bar Text", WS_OVERLAPPEDWINDOW,
          txthght * 3, txthght * 2, txthght * 30, txthght * 21,
          NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) return 0;
  ShowWindow(hwnd, nShowCmd);
  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
} 



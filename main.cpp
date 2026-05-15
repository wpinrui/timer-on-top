#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define IDM_QUIT    2001
#define IDT_TIMER   1

static HWND            g_hwnd;
static NOTIFYICONDATAW g_nid;
static bool            g_running   = false;
static ULONGLONG       g_startTick = 0;
static ULONGLONG       g_elapsed   = 0;   // accumulated ms
static int             g_hover     = -1;  // 0=start 1=stop 2=reset
static RECT            g_btn[3]    = {};

static ULONGLONG Elapsed() {
    ULONGLONG e = g_elapsed;
    if (g_running) e += GetTickCount64() - g_startTick;
    return e;
}

static void DrawScene(HDC hdc, RECT* rc) {
    int W = rc->right, H = rc->bottom;
    int btnH = 36;
    int timerH = H - btnH;

    // ── background ───────────────────────────────────────────────
    HBRUSH bgBrush = CreateSolidBrush(RGB(24, 24, 24));
    FillRect(hdc, rc, bgBrush);
    DeleteObject(bgBrush);

    // ── timer text ───────────────────────────────────────────────
    ULONGLONG ms  = Elapsed();
    ULONGLONG cs  = (ms / 10)    % 100;
    ULONGLONG s   = (ms / 1000)  % 60;
    ULONGLONG m   = (ms / 60000) % 60;
    ULONGLONG hr  =  ms / 3600000ULL;

    char main_s[32], frac_s[8];
    if (hr > 0)
        sprintf_s(main_s, "%llu:%02llu:%02llu", hr, m, s);
    else
        sprintf_s(main_s, "%llu:%02llu", m, s);
    sprintf_s(frac_s, ".%02llu", cs);

    SetBkMode(hdc, TRANSPARENT);
    COLORREF GREEN = RGB(0, 255, 65);

    int mainFH = timerH * 56 / 100;
    int fracFH = timerH * 36 / 100;

    HFONT fMain = CreateFontA(mainFH, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    HFONT fFrac = CreateFontA(fracFH, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

    SIZE szMain, szFrac;
    SelectObject(hdc, fMain);
    GetTextExtentPoint32A(hdc, main_s, (int)strlen(main_s), &szMain);
    SelectObject(hdc, fFrac);
    GetTextExtentPoint32A(hdc, frac_s, (int)strlen(frac_s), &szFrac);

    int totalW = szMain.cx + szFrac.cx;
    int x0     = (W - totalW) / 2;
    int mainY  = (timerH - szMain.cy) / 2;
    int fracY  = mainY + szMain.cy - szFrac.cy;

    SetTextColor(hdc, GREEN);
    SelectObject(hdc, fMain);
    TextOutA(hdc, x0, mainY, main_s, (int)strlen(main_s));
    SelectObject(hdc, fFrac);
    TextOutA(hdc, x0 + szMain.cx, fracY, frac_s, (int)strlen(frac_s));

    DeleteObject(fMain);
    DeleteObject(fFrac);

    // ── separator ────────────────────────────────────────────────
    HPEN sep = CreatePen(PS_SOLID, 1, RGB(55, 55, 55));
    HPEN old = (HPEN)SelectObject(hdc, sep);
    MoveToEx(hdc, 0, timerH, NULL); LineTo(hdc, W, timerH);
    SelectObject(hdc, old); DeleteObject(sep);

    // ── buttons ──────────────────────────────────────────────────
    const char* labels[] = { "START", "STOP", "RESET" };
    int bw = W / 3;
    for (int i = 0; i < 3; i++) {
        g_btn[i] = { bw * i, timerH, (i < 2 ? bw * (i + 1) : W), H };
    }

    HFONT fBtn = CreateFontA(13, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    SelectObject(hdc, fBtn);

    for (int i = 0; i < 3; i++) {
        bool hov = (g_hover == i);
        HBRUSH bb = CreateSolidBrush(hov ? RGB(44, 44, 44) : RGB(24, 24, 24));
        FillRect(hdc, &g_btn[i], bb); DeleteObject(bb);

        if (i > 0) {
            HPEN vp = CreatePen(PS_SOLID, 1, RGB(55, 55, 55));
            HPEN vold = (HPEN)SelectObject(hdc, vp);
            MoveToEx(hdc, g_btn[i].left, timerH, NULL);
            LineTo(hdc, g_btn[i].left, H);
            SelectObject(hdc, vold); DeleteObject(vp);
        }

        SetTextColor(hdc, hov ? GREEN : RGB(170, 170, 170));
        DrawTextA(hdc, labels[i], -1, &g_btn[i],
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    DeleteObject(fBtn);
}

static void ShowMenu(HWND hwnd) {
    POINT pt; GetCursorPos(&pt);
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, IDM_QUIT, L"Quit Timer On Top");
    SetForegroundWindow(hwnd);
    TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(m);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        ZeroMemory(&g_nid, sizeof(g_nid));
        g_nid.cbSize           = sizeof(g_nid);
        g_nid.hWnd             = hwnd;
        g_nid.uID              = 1;
        g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
        lstrcpyW(g_nid.szTip, L"Timer On Top");
        Shell_NotifyIconW(NIM_ADD, &g_nid);
        SetTimer(hwnd, IDT_TIMER, 10, NULL);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_TIMER);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;

    case WM_TIMER:
        if (g_running) InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HGDIOBJ ob  = SelectObject(mem, bmp);
        DrawScene(mem, &rc);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, ob); DeleteObject(bmp); DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (PtInRect(&g_btn[0], pt) && !g_running) {
            g_startTick = GetTickCount64();
            g_running   = true;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (PtInRect(&g_btn[1], pt) && g_running) {
            g_elapsed  += GetTickCount64() - g_startTick;
            g_running   = false;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (PtInRect(&g_btn[2], pt)) {
            g_running  = false;
            g_elapsed  = 0;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        int prev = g_hover; g_hover = -1;
        for (int i = 0; i < 3; i++) if (PtInRect(&g_btn[i], pt)) { g_hover = i; break; }
        if (prev != g_hover) InvalidateRect(hwnd, NULL, FALSE);
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        if (g_hover != -1) { g_hover = -1; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;

    case WM_RBUTTONUP:
        ShowMenu(hwnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDM_QUIT) DestroyWindow(hwnd);
        return 0;

    case WM_TRAYICON:
        if (lp == WM_RBUTTONUP) ShowMenu(hwnd);
        else if (lp == WM_LBUTTONDBLCLK)
            ShowWindow(hwnd, IsWindowVisible(hwnd) ? SW_HIDE : SW_SHOW);
        return 0;

    case WM_DPICHANGED: {
        RECT* r = (RECT*)lp;
        SetWindowPos(hwnd, NULL, r->left, r->top,
            r->right - r->left, r->bottom - r->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wp, lp);
        if (hit == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            for (int i = 0; i < 3; i++) if (PtInRect(&g_btn[i], pt)) return hit;
            return HTCAPTION;
        }
        return hit;
    }

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"TimerOnTop";
    RegisterClassExW(&wc);

    UINT dpi = GetDpiForSystem();
    int  w   = MulDiv(340, dpi, 96);
    int  h   = MulDiv(120, dpi, 96);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"TimerOnTop", L"Timer On Top",
        WS_POPUP,
        100, 100, w, h,
        NULL, NULL, hInst, NULL
    );

    g_hwnd = hwnd;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

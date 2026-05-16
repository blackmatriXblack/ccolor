#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>



#define MAX_COLORS 32
#define IDC_COLOR1    1001
#define IDC_COLOR2    1002
#define IDC_BTN_MIX   1003
#define IDC_TRACKBAR  1004
#define IDC_LABEL_R1  1005
#define IDC_LABEL_G1  1006
#define IDC_LABEL_B1  1007
#define IDC_LABEL_R2  1008
#define IDC_LABEL_G2  1009
#define IDC_LABEL_B2  1010
#define IDC_LABEL_RR  1011
#define IDC_LABEL_GR  1012
#define IDC_LABEL_BR  1013
#define IDC_RATIO_LBL 1014
#define IDC_COLORNAME 1015

typedef struct {
    const char *name;
    int r, g, b;
} ColorDef;

static ColorDef palette[MAX_COLORS] = {
    {"Red",        255,   0,   0},
    {"Green",        0, 255,   0},
    {"Blue",         0,   0, 255},
    {"Yellow",     255, 255,   0},
    {"Cyan",         0, 255, 255},
    {"Magenta",    255,   0, 255},
    {"White",      255, 255, 255},
    {"Black",        0,   0,   0},
    {"Orange",     255, 165,   0},
    {"Purple",     128,   0, 128},
    {"Pink",       255, 192, 203},
    {"Brown",      139,  69,  19},
    {"Gray",       128, 128, 128},
    {"Lime",        50, 205,  50},
    {"Teal",         0, 128, 128},
    {"Navy",         0,   0, 128},
    {"Maroon",     128,   0,   0},
    {"Olive",      128, 128,   0},
    {"Silver",     192, 192, 192},
    {"Coral",      255, 127,  80},
    {"Gold",       255, 215,   0},
    {"Violet",     238, 130, 238},
    {"Indigo",      75,   0, 130},
    {"Turquoise",   64, 224, 208},
    {"Salmon",     250, 128, 114},
    {"Plum",       221, 160, 221},
    {"Tan",        210, 180, 140},
    {"Khaki",      240, 230, 140},
    {"Lavender",   230, 230, 250},
    {"Mint",       189, 252, 201},
    {"Peach",      255, 229, 180},
    {"SkyBlue",    135, 206, 235},
};

typedef struct {
    HBRUSH hBrush1, hBrush2, hBrushResult;
    int cr, cg, cb;
    int r1, g1, b1, r2, g2, b2;
    int rr, gr, br;
    HWND hCombo1, hCombo2, hSlider;
    HFONT hFont, hFontBold;
} AppState;

static AppState g_state;

static const char *nearest_color_name(int r, int g, int b) {
    double best = INFINITY;
    const char *best_name = NULL;
    for (int i = 0; i < MAX_COLORS; i++) {
        double dr = palette[i].r - r, dg = palette[i].g - g, db = palette[i].b - b;
        double dist = dr * dr + dg * dg + db * db;
        if (dist < best) { best = dist; best_name = palette[i].name; }
    }
    return best_name;
}

static void update_result(HWND hwnd) {
    g_state.r1 = palette[g_state.cr].r;
    g_state.g1 = palette[g_state.cr].g;
    g_state.b1 = palette[g_state.cr].b;
    g_state.r2 = palette[g_state.cg].r;
    g_state.g2 = palette[g_state.cg].g;
    g_state.b2 = palette[g_state.cg].b;

    int ratio = (int)SendMessage(g_state.hSlider, TBM_GETPOS, 0, 0);
    g_state.rr = (g_state.r1 * (100 - ratio) + g_state.r2 * ratio) / 100;
    g_state.gr = (g_state.g1 * (100 - ratio) + g_state.g2 * ratio) / 100;
    g_state.br = (g_state.b1 * (100 - ratio) + g_state.b2 * ratio) / 100;

    if (g_state.hBrush1)  DeleteObject(g_state.hBrush1);
    if (g_state.hBrush2)  DeleteObject(g_state.hBrush2);
    if (g_state.hBrushResult) DeleteObject(g_state.hBrushResult);
    g_state.hBrush1    = CreateSolidBrush(RGB(g_state.r1, g_state.g1, g_state.b1));
    g_state.hBrush2    = CreateSolidBrush(RGB(g_state.r2, g_state.g2, g_state.b2));
    g_state.hBrushResult = CreateSolidBrush(RGB(g_state.rr, g_state.gr, g_state.br));

    char buf[64];
    sprintf(buf, "R: %d  G: %d  B: %d", g_state.r1, g_state.g1, g_state.b1);
    SetDlgItemText(hwnd, IDC_LABEL_R1, buf);
    sprintf(buf, "R: %d  G: %d  B: %d", g_state.r2, g_state.g2, g_state.b2);
    SetDlgItemText(hwnd, IDC_LABEL_G1, buf);
    sprintf(buf, "R: %d  G: %d  B: %d", g_state.rr, g_state.gr, g_state.br);
    SetDlgItemText(hwnd, IDC_LABEL_RR, buf);

    sprintf(buf, "Mix Ratio: %d%% / %d%%", 100 - ratio, ratio);
    SetDlgItemText(hwnd, IDC_RATIO_LBL, buf);

    const char *cname = nearest_color_name(g_state.rr, g_state.gr, g_state.br);
    char namebuf[128];
    sprintf(namebuf, "Result Color:  %s    RGB(%d, %d, %d)", cname, g_state.rr, g_state.gr, g_state.br);
    SetDlgItemText(hwnd, IDC_COLORNAME, namebuf);

    InvalidateRect(hwnd, NULL, TRUE);
}

static void init_controls(HWND hwnd, int cx, int cy) {
    HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    int left_margin = 20, top = 20, combo_w = 160, combo_h = 200;
    int gap = 30;

    CreateWindow("STATIC", "Color 1", WS_CHILD | WS_VISIBLE,
        left_margin, top, 100, 20, hwnd, NULL, hi, NULL);
    g_state.hCombo1 = CreateWindow("COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        left_margin, top + 20, combo_w, combo_h, hwnd, (HMENU)IDC_COLOR1, hi, NULL);

    CreateWindow("STATIC", "Color 2", WS_CHILD | WS_VISIBLE,
        left_margin + combo_w + gap, top, 100, 20, hwnd, NULL, hi, NULL);
    g_state.hCombo2 = CreateWindow("COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        left_margin + combo_w + gap, top + 20, combo_w, combo_h, hwnd, (HMENU)IDC_COLOR2, hi, NULL);

    for (int i = 0; i < MAX_COLORS; i++) {
        SendMessage(g_state.hCombo1, CB_ADDSTRING, 0, (LPARAM)palette[i].name);
        SendMessage(g_state.hCombo2, CB_ADDSTRING, 0, (LPARAM)palette[i].name);
    }
    SendMessage(g_state.hCombo1, CB_SETCURSEL, 0, 0);
    SendMessage(g_state.hCombo2, CB_SETCURSEL, 3, 0);

    g_state.hSlider = CreateWindow(TRACKBAR_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
        left_margin, top + 55, combo_w * 2 + gap, 35, hwnd, (HMENU)IDC_TRACKBAR, hi, NULL);
    SendMessage(g_state.hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendMessage(g_state.hSlider, TBM_SETPOS, TRUE, 50);

    CreateWindow("BUTTON", "MIX COLORS",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        left_margin, top + 90, combo_w * 2 + gap, 36, hwnd, (HMENU)IDC_BTN_MIX, hi, NULL);

    CreateWindow("STATIC", "Mix Ratio: 50% / 50%",
        WS_CHILD | WS_VISIBLE,
        left_margin, top + 135, 200, 20, hwnd, (HMENU)IDC_RATIO_LBL, hi, NULL);

    CreateWindow("STATIC", "R: 255  G: 0  B: 0",
        WS_CHILD | WS_VISIBLE,
        left_margin, top + 160, 250, 20, hwnd, (HMENU)IDC_LABEL_R1, hi, NULL);
    CreateWindow("STATIC", "R: 255  G: 255  B: 0",
        WS_CHILD | WS_VISIBLE,
        left_margin, top + 180, 250, 20, hwnd, (HMENU)IDC_LABEL_G1, hi, NULL);
    CreateWindow("STATIC", "R: 255  G: 127  B: 0",
        WS_CHILD | WS_VISIBLE,
        left_margin, top + 200, 250, 20, hwnd, (HMENU)IDC_LABEL_RR, hi, NULL);

    CreateWindow("STATIC", "Result Color:  Orange    RGB(255, 127, 0)",
        WS_CHILD | WS_VISIBLE,
        left_margin, top + 225, 400, 22, hwnd, (HMENU)IDC_COLORNAME, hi, NULL);

    for (int id = IDC_LABEL_R1; id <= IDC_COLORNAME; id++) {
        SendMessage(GetDlgItem(hwnd, id), WM_SETFONT, (WPARAM)g_state.hFont, TRUE);
    }
    SendMessage(GetDlgItem(hwnd, IDC_COLORNAME), WM_SETFONT, (WPARAM)g_state.hFontBold, TRUE);

    g_state.r1 = 255; g_state.g1 = 0;   g_state.b1 = 0;
    g_state.r2 = 255; g_state.g2 = 255; g_state.b2 = 0;
    g_state.rr = 255; g_state.gr = 127; g_state.br = 0;
    g_state.cr = 0; g_state.cg = 3;
    g_state.hBrush1 = CreateSolidBrush(RGB(255, 0, 0));
    g_state.hBrush2 = CreateSolidBrush(RGB(255, 255, 0));
    g_state.hBrushResult = CreateSolidBrush(RGB(255, 127, 0));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_state.hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        g_state.hFontBold = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        RECT rc;
        GetClientRect(hwnd, &rc);
        init_controls(hwnd, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        HWND hctl = (HWND)lp;
        int id = GetDlgCtrlID(hctl);
        SetBkMode(hdc, TRANSPARENT);
        if (id == IDC_COLORNAME) {
            SetTextColor(hdc, RGB(g_state.rr, g_state.gr, g_state.br));
        }
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_BTN_MIX) {
            g_state.cr = (int)SendMessage(g_state.hCombo1, CB_GETCURSEL, 0, 0);
            g_state.cg = (int)SendMessage(g_state.hCombo2, CB_GETCURSEL, 0, 0);
            update_result(hwnd);
        }
        break;
    case WM_HSCROLL:
        update_result(hwnd);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH hBg = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        int color_top = 265;
        int swatch_w = (rc.right - 80) / 3;
        int swatch_h = 130;
        int x0 = 20, x1 = x0 + swatch_w + 20, x2 = x1 + swatch_w + 20;

        RECT r;
        SetRect(&r, x0, color_top, x0 + swatch_w, color_top + swatch_h);
        FillRect(hdc, &r, g_state.hBrush1);
        FrameRect(hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));

        SetRect(&r, x1, color_top, x1 + swatch_w, color_top + swatch_h);
        FillRect(hdc, &r, g_state.hBrush2);
        FrameRect(hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));

        SetRect(&r, x2, color_top, x2 + swatch_w, color_top + swatch_h);
        FillRect(hdc, &r, g_state.hBrushResult);
        FrameRect(hdc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        SelectObject(hdc, g_state.hFontBold);

        char label[64];
        SetRect(&r, x0, color_top + swatch_h + 5, x0 + swatch_w, color_top + swatch_h + 28);
        sprintf(label, "%s", palette[g_state.cr].name);
        DrawText(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetRect(&r, x1, color_top + swatch_h + 5, x1 + swatch_w, color_top + swatch_h + 28);
        sprintf(label, "%s", palette[g_state.cg].name);
        DrawText(hdc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetRect(&r, x2, color_top + swatch_h + 5, x2 + swatch_w, color_top + swatch_h + 28);
        const char *cname = nearest_color_name(g_state.rr, g_state.gr, g_state.br);
        DrawText(hdc, cname, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DrawText(hdc, "+", -1, &(RECT){x0 + swatch_w + 3, color_top + swatch_h / 2 - 15,
            x1 - 3, color_top + swatch_h / 2 + 15}, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DrawText(hdc, "=", -1, &(RECT){x1 + swatch_w + 3, color_top + swatch_h / 2 - 15,
            x2 - 3, color_top + swatch_h / 2 + 15}, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (g_state.hBrush1) DeleteObject(g_state.hBrush1);
        if (g_state.hBrush2) DeleteObject(g_state.hBrush2);
        if (g_state.hBrushResult) DeleteObject(g_state.hBrushResult);
        if (g_state.hFont) DeleteObject(g_state.hFont);
        if (g_state.hFontBold) DeleteObject(g_state.hFontBold);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ColorMixGUI";

    RegisterClass(&wc);

    int w = 660, h = 480;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    HWND hwnd = CreateWindow("ColorMixGUI", "ColorMix - GUI Color Mixer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, w, h, NULL, NULL, hi, NULL);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

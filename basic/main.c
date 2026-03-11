#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define APP_CLASS_NAME "OvalWindowClass"
#define WINDOW_W 520
#define WINDOW_H 420
#define IDC_CLOSE 1001

static void ApplyOvalShape(HWND hwnd) {
    RECT rc;
    HRGN region;
    GetClientRect(hwnd, &rc);
    region = CreateEllipticRgn(0, 0, rc.right, rc.bottom);
    SetWindowRgn(hwnd, region, TRUE);
}

static void LayoutControls(HWND hwnd) {
    RECT rc;
    HWND button;
    GetClientRect(hwnd, &rc);
    button = GetDlgItem(hwnd, IDC_CLOSE);
    if (button) MoveWindow(button, (rc.right - 90) / 2, 24, 90, 32, TRUE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("BUTTON", "CLOSE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 90, 32, hwnd, (HMENU)(INT_PTR)IDC_CLOSE,
            ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
        ApplyOvalShape(hwnd);
        LayoutControls(hwnd);
        return 0;

    case WM_SIZE:
        ApplyOvalShape(hwnd);
        LayoutControls(hwnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CLOSE) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_LBUTTONDOWN: {
        POINT pt;
        HWND child;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        child = ChildWindowFromPoint(hwnd, pt);
        if (child != GetDlgItem(hwnd, IDC_CLOSE)) {
            ReleaseCapture();
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        return 0;
    }

    case WM_RBUTTONUP:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmdLine, int showCmd) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    (void)prev;
    (void)cmdLine;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassA(&wc)) return 1;

    hwnd = CreateWindowExA(0, APP_CLASS_NAME, "Oval", WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_W, WINDOW_H,
        NULL, NULL, instance, NULL);
    if (!hwnd) return 1;

    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

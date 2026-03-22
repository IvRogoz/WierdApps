#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

#define APP_CLASS_NAME "ImageShapeWindowClass"
#define IDC_CLOSE 1001
#define TRANSPARENT_COLOR RGB(255, 0, 255)

static HBITMAP gBitmap;
static int gBitmapW;
static int gBitmapH;

static int LoadShapeBitmap(const char *path) {
    BITMAP bm;
    gBitmap = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!gBitmap) return 0;
    if (!GetObject(gBitmap, sizeof(bm), &bm)) return 0;
    gBitmapW = bm.bmWidth;
    gBitmapH = bm.bmHeight;
    return 1;
}

static HRGN BuildRegionFromBitmap(HBITMAP bitmap, COLORREF transparentColor) {
    BITMAPINFO bmi;
    uint32_t *pixels;
    HRGN result;
    int x;
    int y;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = gBitmapW;
    bmi.bmiHeader.biHeight = -gBitmapH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    pixels = (uint32_t *)malloc((size_t)gBitmapW * (size_t)gBitmapH * sizeof(uint32_t));
    if (!pixels) return CreateRectRgn(0, 0, gBitmapW, gBitmapH);

    {
        HDC hdc = GetDC(NULL);
        if (!GetDIBits(hdc, bitmap, 0, (UINT)gBitmapH, pixels, &bmi, DIB_RGB_COLORS)) {
            ReleaseDC(NULL, hdc);
            free(pixels);
            return CreateRectRgn(0, 0, gBitmapW, gBitmapH);
        }
        ReleaseDC(NULL, hdc);
    }

    result = CreateRectRgn(0, 0, 0, 0);
    for (y = 0; y < gBitmapH; ++y) {
        x = 0;
        while (x < gBitmapW) {
            HRGN run;
            COLORREF color;
            while (x < gBitmapW) {
                uint32_t p = pixels[(size_t)y * (size_t)gBitmapW + (size_t)x];
                color = RGB((p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff);
                if (color != transparentColor) break;
                ++x;
            }
            if (x >= gBitmapW) break;
            {
                int start = x;
                while (x < gBitmapW) {
                    uint32_t p = pixels[(size_t)y * (size_t)gBitmapW + (size_t)x];
                    color = RGB((p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff);
                    if (color == transparentColor) break;
                    ++x;
                }
                run = CreateRectRgn(start, y, x, y + 1);
                CombineRgn(result, result, run, RGN_OR);
                DeleteObject(run);
            }
        }
    }
    free(pixels);
    return result;
}

static void ApplyImageShape(HWND hwnd) {
    HRGN region;
    if (!gBitmap) return;
    region = BuildRegionFromBitmap(gBitmap, TRANSPARENT_COLOR);
    SetWindowRgn(hwnd, region, TRUE);
}

static void LayoutControls(HWND hwnd) {
    HWND button = GetDlgItem(hwnd, IDC_CLOSE);
    if (button) MoveWindow(button, 70, 70, 120, 50, TRUE);
}

static void PaintBitmap(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc;
    HDC memdc;
    HGDIOBJ oldBitmap;
    hdc = BeginPaint(hwnd, &ps);
    memdc = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(memdc, gBitmap);
    BitBlt(hdc, 0, 0, gBitmapW, gBitmapH, memdc, 0, 0, SRCCOPY);
    SelectObject(memdc, oldBitmap);
    DeleteDC(memdc);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        if (!LoadShapeBitmap("shape.bmp")) {
            MessageBoxA(hwnd, "Missing drivenbyimage\\shape.bmp", "shape.bmp not found", MB_ICONERROR);
            return -1;
        }
        CreateWindowA("BUTTON", "CLOSE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            70, 70, 120, 50, hwnd, (HMENU)(INT_PTR)IDC_CLOSE,
            ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
        SetWindowPos(hwnd, NULL, 0, 0, gBitmapW, gBitmapH, SWP_NOMOVE | SWP_NOZORDER);
        ApplyImageShape(hwnd);
        LayoutControls(hwnd);
        return 0;

    case WM_SIZE:
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

    case WM_PAINT:
        PaintBitmap(hwnd);
        return 0;

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
        if (gBitmap) DeleteObject(gBitmap);
        gBitmap = NULL;
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

    hwnd = CreateWindowExA(0, APP_CLASS_NAME, "Driven By Image", WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 320, NULL, NULL, instance, NULL);
    if (!hwnd) return 1;

    ShowWindow(hwnd, showCmd);
    UpdateWindow(hwnd);
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

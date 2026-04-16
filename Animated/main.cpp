#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

using namespace Gdiplus;

namespace {

const wchar_t kClassName[] = L"DogMascotWindow";
const wchar_t kSpriteSheetName[] = L"Dog_medium.png";
const UINT_PTR kTimerId = 1;
const UINT kTickMs = 100;
const int kFrameWidth = 60;
const int kFrameHeight = 38;
const int kScale = 3;
const int kRows = 6;
const int kCols = 6;

enum AnimationRow {
    ROW_BARK = 0,
    ROW_WALK = 1,
    ROW_RUN = 2,
    ROW_SIT_TRANSITION = 3,
    ROW_IDLE_SIT = 4,
    ROW_IDLE_STAND = 5
};

struct MascotApp {
    HWND hwnd;
    ULONG_PTR gdiplusToken;
    Bitmap *sheet;
    HDC memdc;
    HBITMAP dib;
    HBITMAP oldBitmap;
    uint32_t *pixels;
    RECT workArea;
    int width;
    int height;
    int x;
    int y;
    int facing;
    int row;
    int frame;
    int frameElapsedMs;
    int actionLoopsLeft;
    int barkReturnRow;
    int transitionTargetRow;
    bool transitionReverse;
    bool dragging;
    POINT dragOffset;
};

const int kFrameCounts[kRows] = {4, 6, 5, 3, 4, 4};
const int kFrameDurationsMs[kRows] = {120, 110, 80, 140, 260, 240};

void FreeResources(MascotApp *app) {
    if (!app) return;
    if (app->memdc) {
        if (app->oldBitmap) SelectObject(app->memdc, app->oldBitmap);
        if (app->dib) DeleteObject(app->dib);
        DeleteDC(app->memdc);
    }
    delete app->sheet;
    if (app->gdiplusToken) GdiplusShutdown(app->gdiplusToken);
    free(app);
}

bool IsOpaquePixel(MascotApp *app, int x, int y) {
    if (!app) return false;
    if (x < 0 || y < 0 || x >= app->width || y >= app->height) return false;
    return ((app->pixels[y * app->width + x] >> 24) & 0xff) > 16;
}

void ClampToWorkArea(MascotApp *app) {
    int maxX;
    int maxY;
    if (!app) return;
    maxX = app->workArea.right - app->width;
    maxY = app->workArea.bottom - app->height;
    if (app->x < app->workArea.left) app->x = app->workArea.left;
    if (app->x > maxX) app->x = maxX;
    if (app->y < app->workArea.top) app->y = app->workArea.top;
    if (app->y > maxY) app->y = maxY;
}

void SwitchAnimation(MascotApp *app, int row, int loops) {
    if (!app) return;
    app->row = row;
    app->frameElapsedMs = 0;
    app->actionLoopsLeft = loops;
    if (row == ROW_SIT_TRANSITION && app->transitionReverse) {
        app->frame = kFrameCounts[row] - 1;
    } else {
        app->frame = 0;
    }
}

void ChooseNextAction(MascotApp *app) {
    int roll;
    if (!app) return;

    if (app->row == ROW_IDLE_SIT) {
        roll = rand() % 100;
        if (roll < 25) {
            app->barkReturnRow = ROW_IDLE_SIT;
            app->transitionReverse = false;
            SwitchAnimation(app, ROW_BARK, 1);
        } else if (roll < 60) {
            app->transitionReverse = false;
            SwitchAnimation(app, ROW_IDLE_SIT, 2 + rand() % 4);
        } else {
            app->transitionReverse = true;
            app->transitionTargetRow = ROW_IDLE_STAND;
            SwitchAnimation(app, ROW_SIT_TRANSITION, 1);
        }
        return;
    }

    roll = rand() % 100;
    if (roll < 15) {
        app->barkReturnRow = ROW_IDLE_STAND;
        app->transitionReverse = false;
        SwitchAnimation(app, ROW_BARK, 1);
    } else if (roll < 55) {
        app->facing = (rand() & 1) ? 1 : -1;
        SwitchAnimation(app, ROW_WALK, 2 + rand() % 5);
    } else if (roll < 75) {
        app->facing = (rand() & 1) ? 1 : -1;
        SwitchAnimation(app, ROW_RUN, 2 + rand() % 4);
    } else if (roll < 90) {
        app->transitionReverse = false;
        app->transitionTargetRow = ROW_IDLE_SIT;
        SwitchAnimation(app, ROW_SIT_TRANSITION, 1);
    } else {
        app->transitionReverse = false;
        SwitchAnimation(app, ROW_IDLE_STAND, 2 + rand() % 4);
    }
}

void UpdateMovement(MascotApp *app){
    int speed = 0;
    int maxX;
    if (!app) return;
    if (app->dragging) return;

    if (app->row == ROW_WALK) speed = 5;
    if (app->row == ROW_RUN) speed = 10;
    if (!speed) return;

    app->x += speed * app->facing;
    maxX = app->workArea.right - app->width;
    if (app->x <= app->workArea.left) {
        app->x = app->workArea.left;
        app->facing = 1;
    } else if (app->x >= maxX) {
        app->x = maxX;
        app->facing = -1;
    }
}

void AdvanceAnimation(MascotApp *app) {
    if (!app) return;

    app->frameElapsedMs += kTickMs;
    if (app->frameElapsedMs < kFrameDurationsMs[app->row]) return;
    app->frameElapsedMs = 0;

    if (app->row == ROW_SIT_TRANSITION && app->transitionReverse) {
        --app->frame;
        if (app->frame < 0) {
            app->transitionReverse = false;
            SwitchAnimation(app, app->transitionTargetRow, 2 + rand() % 4);
        }
        return;
    }

    ++app->frame;
    if (app->frame < kFrameCounts[app->row]) return;

    app->frame = 0;
    if (app->row == ROW_BARK) {
        SwitchAnimation(app, app->barkReturnRow, 2 + rand() % 4);
        return;
    }
    if (app->row == ROW_SIT_TRANSITION) {
        SwitchAnimation(app, app->transitionTargetRow, 2 + rand() % 4);
        return;
    }

    if (app->actionLoopsLeft > 0) --app->actionLoopsLeft;
    if (app->actionLoopsLeft > 0) return;

    if (app->row == ROW_WALK || app->row == ROW_RUN || app->row == ROW_IDLE_STAND || app->row == ROW_IDLE_SIT) {
        ChooseNextAction(app);
    }
}

void RenderMascot(MascotApp *app) {
    Graphics graphics(app->memdc);
    Rect src;
    Rect dst;
    SIZE size;
    POINT srcPt;
    BLENDFUNCTION blend;

    ZeroMemory(app->pixels, (size_t)app->width * (size_t)app->height * sizeof(uint32_t));

    graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.SetCompositingQuality(CompositingQualityHighSpeed);
    graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
    graphics.Clear(Color(0, 0, 0, 0));

    src = Rect(app->frame * kFrameWidth, app->row * kFrameHeight, kFrameWidth, kFrameHeight);
    if (app->facing > 0) {
        graphics.TranslateTransform((REAL)app->width, 0.0f);
        graphics.ScaleTransform(-1.0f, 1.0f);
    }
    dst = Rect(0, 0, app->width, app->height);
    graphics.DrawImage(app->sheet, dst, src.X, src.Y, src.Width, src.Height, UnitPixel);
    graphics.ResetTransform();

    size.cx = app->width;
    size.cy = app->height;
    srcPt.x = 0;
    srcPt.y = 0;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(app->hwnd, NULL, (POINT *)&app->x, &size, app->memdc, &srcPt, 0, &blend, ULW_ALPHA);
}

bool CreateBackBuffer(MascotApp *app) {
    BITMAPINFO bmi;
    HDC screen = GetDC(NULL);
    if (!screen) return false;

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = app->width;
    bmi.bmiHeader.biHeight = -app->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    app->memdc = CreateCompatibleDC(screen);
    app->dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, (void **)&app->pixels, NULL, 0);
    ReleaseDC(NULL, screen);
    if (!app->memdc || !app->dib || !app->pixels) return false;

    app->oldBitmap = (HBITMAP)SelectObject(app->memdc, app->dib);
    return true;
}

bool LoadSpriteSheet(MascotApp *app) {
    WCHAR modulePath[MAX_PATH];
    WCHAR *slash;
    WCHAR fullPath[MAX_PATH];

    if (!GetModuleFileNameW(NULL, modulePath, MAX_PATH)) return false;
    slash = wcsrchr(modulePath, L'\\');
    if (!slash) return false;
    slash[1] = L'\0';
    if (FAILED(StringCchCopyW(fullPath, MAX_PATH, modulePath))) return false;
    if (FAILED(StringCchCatW(fullPath, MAX_PATH, kSpriteSheetName))) return false;

    app->sheet = new Bitmap(fullPath);
    if (!app->sheet) return false;
    if (app->sheet->GetLastStatus() != Ok) return false;
    return true;
}

void SnapToGround(MascotApp *app) {
    app->y = app->workArea.bottom - app->height - 8;
    ClampToWorkArea(app);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MascotApp *app = (MascotApp *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW *create = (CREATESTRUCTW *)lParam;
        app = (MascotApp *)create->lpCreateParams;
        app->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app);
        SetTimer(hwnd, kTimerId, kTickMs, NULL);
        RenderMascot(app);
        return 0;
    }

    case WM_TIMER:
        if (wParam == kTimerId && app) {
            UpdateMovement(app);
            AdvanceAnimation(app);
            if(!app->dragging) SnapToGround(app);
            RenderMascot(app);
        }
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        if (app) {
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &app->workArea, 0);
            SnapToGround(app);
            RenderMascot(app);
        }
        return 0;

    case WM_NCHITTEST:
        if (app) {
            POINT pt = {GET_X_LPARAM(lParam) - app->x, GET_Y_LPARAM(lParam) - app->y};
            return IsOpaquePixel(app, pt.x, pt.y) ? HTCLIENT : HTTRANSPARENT;
        }
        break;

    case WM_LBUTTONDOWN:
        if (app) {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (IsOpaquePixel(app, pt.x, pt.y)) {
                app->dragging = true;
                app->dragOffset = pt;
                SetCapture(hwnd);
            }
        }
        return 0;

    case WM_MOUSEMOVE:
        if (app && app->dragging) {
            POINT screen = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(hwnd, &screen);
            app->x = screen.x - app->dragOffset.x;
            app->y = screen.y - app->dragOffset.y;
            ClampToWorkArea(app);
            RenderMascot(app);
        }
        return 0;

    case WM_LBUTTONUP:
        if (app && app->dragging) {
            app->dragging = false;
            ReleaseCapture();
            SnapToGround(app);
            RenderMascot(app);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        if (app) {
            app->barkReturnRow = (app->row == ROW_IDLE_SIT) ? ROW_IDLE_SIT : ROW_IDLE_STAND;
            app->transitionReverse = false;
            SwitchAnimation(app, ROW_BARK, 1);
            RenderMascot(app);
        }
        return 0;

    case WM_RBUTTONUP:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    GdiplusStartupInput startupInput;
    WNDCLASSW wc;
    MSG msg;
    MascotApp *app;

    srand((unsigned int)time(NULL));

    app = (MascotApp *)calloc(1, sizeof(*app));
    if (!app) return 1;

    app->width = kFrameWidth * kScale;
    app->height = kFrameHeight * kScale;
    app->facing = -1;
    app->row = ROW_IDLE_STAND;
    app->frame = 0;
    app->actionLoopsLeft = 3;
    app->barkReturnRow = ROW_IDLE_STAND;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &app->workArea, 0);
    app->x = app->workArea.right - app->width - 24;
    app->y = app->workArea.bottom - app->height - 8;

    if (GdiplusStartup(&app->gdiplusToken, &startupInput, NULL) != Ok) {
        FreeResources(app);
        return 1;
    }
    if (!LoadSpriteSheet(app) || !CreateBackBuffer(app)) {
        FreeResources(app);
        MessageBoxW(NULL, L"Could not load Dog_medium.png or create the layered window buffer.", L"Dog Mascot", MB_ICONERROR);
        return 1;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
    if (!RegisterClassW(&wc)) {
        FreeResources(app);
        return 1;
    }

    app->hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kClassName,
        L"Dog Mascot",
        WS_POPUP,
        app->x, app->y, app->width, app->height,
        NULL, NULL, instance, app);
    if (!app->hwnd) {
        FreeResources(app);
        return 1;
    }

    ShowWindow(app->hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(app->hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    FreeResources(app);
    return (int)msg.wParam;
}

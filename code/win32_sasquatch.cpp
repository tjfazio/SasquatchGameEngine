#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>

#include "win32_util.h"

#define global_variable static
#define internal_function static
#define local_persist static
#define pixel_t uint32_t

global_variable TCHAR szWindowClass[] = _T("sasquatch");
global_variable TCHAR szTitle[] = _T("Sasquatch Game Engine");

typedef struct tagBitmapBuffer {
    BITMAPINFO Info;
    HBITMAP BitmapHandle;
    HDC DeviceContext;
    void *Memory;
} BitmapBuffer;

global_variable BitmapBuffer GlobalBackBuffer;

pixel_t GetFakePixel(int32_t x, int32_t y)
{
    uint8_t blue = x % 256;
    uint8_t green = y % 256;
    uint8_t red = (x + y) % 128;

    return red | (blue << 4) | (green << 8);
}

void Win32_ResizeWindow(
    HDC targetDeviceContext,
    RECT clientRect
    )
{
    if (GlobalBackBuffer.BitmapHandle)
    {
        DeleteObject(GlobalBackBuffer.BitmapHandle);
    }
    if (!GlobalBackBuffer.DeviceContext)
    {
        // TODO(tjfazio) - mess with this for multiple monitors??
        GlobalBackBuffer.DeviceContext = CreateCompatibleDC(targetDeviceContext);
    }

    GlobalBackBuffer.Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GlobalBackBuffer.Info.bmiHeader.biWidth = Win32_GetRectWidth(clientRect);
    GlobalBackBuffer.Info.bmiHeader.biHeight = Win32_GetRectHeight(clientRect);
    GlobalBackBuffer.Info.bmiHeader.biPlanes = 1;
    GlobalBackBuffer.Info.bmiHeader.biBitCount = 8*sizeof(pixel_t);
    GlobalBackBuffer.Info.bmiHeader.biCompression = BI_RGB;    

    GlobalBackBuffer.BitmapHandle = CreateDIBSection(
        targetDeviceContext,
        &GlobalBackBuffer.Info,
        DIB_RGB_COLORS,
        &GlobalBackBuffer.Memory,
        0,
        0
    );

    int32_t width = GlobalBackBuffer.Info.bmiHeader.biWidth;
    int32_t height = GlobalBackBuffer.Info.bmiHeader.biHeight;
    pixel_t *pixels = (pixel_t *)(GlobalBackBuffer.Memory);
    for (int y = 0; y < width; y++)
    {
        for (int x = 0; x < height; x++)
        {
            // TODO(tjfazio) - figure out why this doesn't work
            // pixels[y * width + x] = 0;
            *pixels = GetFakePixel(x, y);
            pixels++;
        }
    }
}

void Win32_PaintWindow(
    HDC targetDeviceContext,
    RECT paintRect
    )
{
    SelectObject(GlobalBackBuffer.DeviceContext, GlobalBackBuffer.BitmapHandle);
    BitBlt(
        targetDeviceContext,
        paintRect.top,
        paintRect.left,
        Win32_GetRectWidth(paintRect),
        Win32_GetRectHeight(paintRect),
        GlobalBackBuffer.DeviceContext,
        paintRect.top,
        paintRect.left,
        SRCCOPY
    );
}

LRESULT WndProc(  
  HWND hWnd,  
  UINT uMsg,  
  WPARAM wParam,  
  LPARAM lParam  
)
{    
    switch (uMsg)
    {
        case WM_SIZE:
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            Win32_ResizeWindow(NULL, clientRect);
            break;
        case WM_PAINT:
            PAINTSTRUCT paint;
            HDC deviceContext;
            deviceContext = BeginPaint(hWnd, &paint);
            Win32_PaintWindow(deviceContext, paint.rcPaint);
            EndPaint(hWnd, &paint);
            break;
        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return 0;
}

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Sasquatch"),
            NULL
        );
        
        return 1;
    }

    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        500,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Sasquatch"),
            NULL
        );

        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>

#include "win32_util.h"
#include "input.cpp"

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
} Win32_BitmapBuffer;

global_variable Win32_BitmapBuffer VideoBackBuffer;
global_variable bool IsApplicationRunning;

// Fake gameplay stuff for prototyping
typedef struct tagAnimationState {
    int32_t XStart;
    int32_t XVelocity;
    int32_t YStart;
    int32_t YVelocity;
    int32_t BlueWidth;
    int32_t GreenWidth;
} AnimationState;


enum VirtualInputCodes : uint8_t {
    VIC_Up = 0,
    VIC_Down = 1,
    VIC_Left = 3,
    VIC_Right = 4
};

global_variable AnimationState TestAnimation;
global_variable SGE_Keyboard g_Keyboard;

void Win32_ResizeWindow(
    HDC targetDeviceContext,
    RECT clientRect
    )
{
    if (VideoBackBuffer.BitmapHandle)
    {
        DeleteObject(VideoBackBuffer.BitmapHandle);
    }
    if (!VideoBackBuffer.DeviceContext)
    {
        // TODO(tjfazio) - mess with this for multiple monitors??
        VideoBackBuffer.DeviceContext = CreateCompatibleDC(targetDeviceContext);
    }

    VideoBackBuffer.Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    VideoBackBuffer.Info.bmiHeader.biWidth = Win32_GetRectWidth(clientRect);
    VideoBackBuffer.Info.bmiHeader.biHeight = Win32_GetRectHeight(clientRect);
    VideoBackBuffer.Info.bmiHeader.biPlanes = 1;
    VideoBackBuffer.Info.bmiHeader.biBitCount = 8 * sizeof(pixel_t);
    VideoBackBuffer.Info.bmiHeader.biCompression = BI_RGB;

    VideoBackBuffer.BitmapHandle = CreateDIBSection(
        VideoBackBuffer.DeviceContext,
        &VideoBackBuffer.Info,
        DIB_RGB_COLORS,
        &VideoBackBuffer.Memory,
        0,
        0
    );

    TestAnimation.XStart = 0;
    TestAnimation.YStart = 0;
    TestAnimation.XVelocity = 0;
    TestAnimation.YVelocity = 0;
    g_Keyboard.ClearState();
}

void Win32_PaintWindow(
    HDC targetDeviceContext,
    RECT paintRect
    )
{
    SelectObject(VideoBackBuffer.DeviceContext, VideoBackBuffer.BitmapHandle);
    BitBlt(
        targetDeviceContext,
        paintRect.top,
        paintRect.left,
        Win32_GetRectWidth(paintRect),
        Win32_GetRectHeight(paintRect),
        VideoBackBuffer.DeviceContext,
        paintRect.top,
        paintRect.left,
        SRCCOPY
    );
}

pixel_t GetFakePixel(int32_t x, int32_t y)
{
    pixel_t blue = 0;
    if (x >= TestAnimation.XStart && x < TestAnimation.XStart + TestAnimation.BlueWidth)
    {
        blue = 0xFF;
    }   

    pixel_t green = 0;
    if (y >= TestAnimation.YStart && y < TestAnimation.YStart + TestAnimation.GreenWidth)
    {
        green = 0xFF;
    }

    pixel_t red = 0;

    return (red << 16) | (green << 8) | blue;
}

void Animate()
{
    int32_t width = VideoBackBuffer.Info.bmiHeader.biWidth;
    int32_t height = VideoBackBuffer.Info.bmiHeader.biHeight;
    pixel_t *pixels = (pixel_t *)(VideoBackBuffer.Memory);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            pixels[y * width + x] = GetFakePixel(x, y);
        }
    }

    // fake game logic
    if (g_Keyboard.IsSet(VIC_Up) && !g_Keyboard.IsSet(VIC_Down))
    {
        TestAnimation.YVelocity = 1;
    }
    else if (g_Keyboard.IsSet(VIC_Down) && !g_Keyboard.IsSet(VIC_Up))
    {
        TestAnimation.YVelocity = -1;
    }
    else
    {
        TestAnimation.YVelocity = 0;
    }

    if (g_Keyboard.IsSet(VIC_Right) && !g_Keyboard.IsSet(VIC_Left))
    {
        TestAnimation.XVelocity = 1;
    }
    else if (g_Keyboard.IsSet(VIC_Left) && !g_Keyboard.IsSet(VIC_Right))
    {
        TestAnimation.XVelocity = -1;
    }
    else
    {
        TestAnimation.XVelocity = 0;
    }

    if ((TestAnimation.XStart < 0 && TestAnimation.XVelocity < 0)
        || (TestAnimation.XStart + TestAnimation.BlueWidth > VideoBackBuffer.Info.bmiHeader.biWidth) && TestAnimation.XVelocity > 0)
    {
        TestAnimation.XVelocity = 0;
    }
    if ((TestAnimation.YStart < 0 && TestAnimation.YVelocity < 0)
        || (TestAnimation.YStart + TestAnimation.GreenWidth > VideoBackBuffer.Info.bmiHeader.biHeight) && TestAnimation.YVelocity > 0)
    {
        TestAnimation.YVelocity = 0;
    }
    TestAnimation.XStart += TestAnimation.XVelocity;
    TestAnimation.YStart += TestAnimation.YVelocity;
}

void Win32_HandleKey(uint32_t virtualKeyCode, bool isKeyDown)
{
    if (virtualKeyCode >= 0 && virtualKeyCode <= 0xFF)
    {
        g_Keyboard.SetState((uint8_t)virtualKeyCode, isKeyDown);
    }
}

LRESULT WndProc(  
  HWND hWnd,  
  UINT uMsg,  
  WPARAM wParam,  
  LPARAM lParam  
)
{    
    uint32_t keyTransitionState = 0;
    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            keyTransitionState = (1 << 31) & lParam;
            Win32_HandleKey((uint32_t)wParam, (keyTransitionState == 0));
            break;
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

void Win32_InitializeDefaultKeyboardMap()
{
    g_Keyboard.MapAction(VIC_Up, 'W', VK_UP);
    g_Keyboard.MapAction(VIC_Down, 'S', VK_DOWN);
    g_Keyboard.MapAction(VIC_Left, 'A', VK_LEFT);
    g_Keyboard.MapAction(VIC_Right, 'D', VK_RIGHT);
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

    Win32_InitializeDefaultKeyboardMap();

    IsApplicationRunning = true;
    TestAnimation.XStart = 0;
    TestAnimation.XVelocity = 0;
    TestAnimation.YStart = 0;
    TestAnimation.YVelocity = 0;
    TestAnimation.BlueWidth = 16;
    TestAnimation.GreenWidth = 32;

    // Windows will clean this HDC up when the game exits
    HDC deviceContext = GetDC(hWnd);

    while (IsApplicationRunning)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                IsApplicationRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Animate();
        
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        Win32_PaintWindow(deviceContext, clientRect);
    }

    return 0;
}
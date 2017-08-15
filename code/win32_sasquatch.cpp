#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>
#include <strsafe.h>
#include <dsound.h>
#include <assert.h>

#include "common.h"
#include "debug.h"
#include "sasquatch.h"
#include "input.h"
#include "win32_util.h"
#include "win32_sound.h"
#include "file.h"

// Unity build ¯\_(ツ)_/¯
#include "win32_debug.cpp"
#include "win32_sound.cpp"
#include "win32_file.cpp"
#include "sasquatch.cpp"
#include "input.cpp"

global_variable TCHAR szWindowClass[] = _T("sasquatch");
global_variable TCHAR szTitle[] = _T("Sasquatch Game Engine");

const uint64_t Win32_PlatformMemorySize = KILOBYTES(64);
const uint64_t Win32_GameStaticMemorySize = GIGABYTES(2);

// TODO: move these structs to a .h file
typedef struct tagBitmapBuffer {
    int32_t Height;
    int32_t Width;
    int32_t Stride;
    void *Memory;
    BITMAPINFO Info;
    HBITMAP BitmapHandle;
    HDC DeviceContext;
} Win32_BitmapBuffer;

global_variable bool g_IsApplicationRunning;
global_variable Win32_BitmapBuffer g_VideoBackBuffer;
// HACK: remove global pointer after fixing input polling code
global_variable SGE_GameState *g_GameState;

internal void Win32_ResizeWindow(
    HDC targetDeviceContext,
    RECT clientRect
    )
{
    if (g_VideoBackBuffer.BitmapHandle)
    {
        DeleteObject(g_VideoBackBuffer.BitmapHandle);
    }
    if (!g_VideoBackBuffer.DeviceContext)
    {
        g_VideoBackBuffer.DeviceContext = CreateCompatibleDC(targetDeviceContext);
    }

    g_VideoBackBuffer.Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_VideoBackBuffer.Info.bmiHeader.biWidth = Win32_GetRectWidth(clientRect);
    g_VideoBackBuffer.Width = g_VideoBackBuffer.Info.bmiHeader.biWidth;
    g_VideoBackBuffer.Stride = g_VideoBackBuffer.Width;
    g_VideoBackBuffer.Info.bmiHeader.biHeight = -1 * Win32_GetRectHeight(clientRect);
    g_VideoBackBuffer.Height = g_VideoBackBuffer.Info.bmiHeader.biHeight;
    g_VideoBackBuffer.Info.bmiHeader.biPlanes = 1;
    g_VideoBackBuffer.Info.bmiHeader.biBitCount = 8 * sizeof(pixel_t);
    g_VideoBackBuffer.Info.bmiHeader.biCompression = BI_RGB;

    g_VideoBackBuffer.BitmapHandle = CreateDIBSection(
        g_VideoBackBuffer.DeviceContext,
        &g_VideoBackBuffer.Info,
        DIB_RGB_COLORS,
        &g_VideoBackBuffer.Memory,
        0,
        0
    );
}

internal void Win32_PaintWindow(
    HDC targetDeviceContext,
    RECT paintRect
    )
{
    SelectObject(g_VideoBackBuffer.DeviceContext, g_VideoBackBuffer.BitmapHandle);
    BitBlt(
        targetDeviceContext,
        paintRect.top,
        paintRect.left,
        Win32_GetRectWidth(paintRect),
        Win32_GetRectHeight(paintRect),
        g_VideoBackBuffer.DeviceContext,
        paintRect.top,
        paintRect.left,
        SRCCOPY
    );
}

void Win32_HandleKey(uint32_t virtualKeyCode, bool isKeyDown)
{
    if (g_GameState == NULL)
    {
        return;
    }
    if (virtualKeyCode >= 0 && virtualKeyCode <= 0xFF)
    {
        g_GameState->Controllers[SGE_Keyboard].UpdateState((uint8_t)virtualKeyCode, isKeyDown);
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

internal void Win32_InitializeDefaultKeyboardMap(SGE_Controller *keyboard)
{
    keyboard->MapAction(SGE_Up, 'W', VK_UP);
    keyboard->MapAction(SGE_Down, 'S', VK_DOWN);
    keyboard->MapAction(SGE_Left, 'A', VK_LEFT);
    keyboard->MapAction(SGE_Right, 'D', VK_RIGHT);
    keyboard->MapAction(SGE_Action1, 'Q', VK_SPACE);
}

internal real32_t Win32_GetElapsedSeconds(LARGE_INTEGER lastTime)
{
    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    int64_t timeDelta = currentTime.QuadPart - lastTime.QuadPart;
    real32_t elapsedSeconds = ((real32_t)timeDelta) / counterFrequency.QuadPart;
    return elapsedSeconds;
}

internal void Win32_InitializeSound(
    Win32_SoundOutput **win32SoundOutputPtr, 
    SGE_SoundBuffer **gameSoundBufferPtr,
    SGE_GameClock *gameClock,
    HWND hWnd)
{        
    uint8_t *platformMemory = (uint8_t *)VirtualAlloc(NULL, Win32_PlatformMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    assert(platformMemory != NULL);
    uint64_t platformMemoryNextByte = 0;

    Win32_SoundOutput *win32SoundOutput = (Win32_SoundOutput *)(platformMemory + platformMemoryNextByte);
    platformMemoryNextByte += sizeof(Win32_SoundOutput);

    int32_t samplesPerFrame = (int32_t)(Win32_SoundSamplesPerSecond / gameClock->TargetFrameRate);
    int32_t soundBufferSize = 4 * Win32_NumChannels * sizeof(sample_t) * samplesPerFrame;
    Win32_InitializeDirectSound(win32SoundOutput, hWnd, Win32_SoundBufferSize, Win32_SoundSamplesPerSecond);
    Win32_ClearSoundBuffer(win32SoundOutput);
    win32SoundOutput->SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    SGE_SoundBuffer *gameSoundBuffer = (SGE_SoundBuffer *)(platformMemory + platformMemoryNextByte);
    platformMemoryNextByte += sizeof(SGE_SoundBuffer);

    gameSoundBuffer->SamplesPerSecond = Win32_SoundSamplesPerSecond;
    gameSoundBuffer->NumChannels = Win32_NumChannels;
    gameSoundBuffer->BufferSize = soundBufferSize;
    gameSoundBuffer->Memory = (sample_t *)(platformMemory + platformMemoryNextByte);
    platformMemoryNextByte += soundBufferSize;
    
    assert(Win32_PlatformMemorySize > platformMemoryNextByte);

    *win32SoundOutputPtr = win32SoundOutput;
    *gameSoundBufferPtr = gameSoundBuffer;
}

internal SGE_GameState *Win32_InitializeGameState()
{
    uint8_t *gameStaticMemory = (uint8_t *)VirtualAlloc(NULL, Win32_GameStaticMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    assert(gameStaticMemory != NULL);
    assert(Win32_GameStaticMemorySize > sizeof(SGE_GameState));
    SGE_GameState *gameState = (SGE_GameState *)gameStaticMemory;
    // HACK: need a global pointer for updating the game controllers from Windows events 
    g_GameState = gameState;

    SGE_Init(gameState);
    Win32_InitializeDefaultKeyboardMap(&gameState->Controllers[SGE_Keyboard]);

    return gameState;
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

    int32_t windowWidth = 1024;
    int32_t windowHeight = 768;

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Sasquatch"),
            NULL
        );
        
        return 1;
    }

    DWORD windowStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
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
    
    Debug_InitializeLog(Debug_Warning, "DebugLog.txt");
    if (!SGE_InitializeFileSystem())
    {
        MessageBox(NULL, 
            _T("File system initializatino failed!"),
            _T("Sasquatch"),
            NULL
        );

        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    g_IsApplicationRunning = true;

    SGE_GameClock gameClock = {};
    
    // TODO: calculate these values based on hardware capabilities
    gameClock.TargetFrameRate = 30;
    gameClock.TargetFrameLengthSeconds = (real32_t)(1.0 / gameClock.TargetFrameRate);
    // TODO: figure out why I need so much padding for sound to work
    gameClock.FrameVariationSeconds = (real32_t)(2.0 * gameClock.TargetFrameLengthSeconds);

    Win32_SoundOutput *win32SoundOutput;
    SGE_SoundBuffer *gameSoundBuffer;
    Win32_InitializeSound(&win32SoundOutput, &gameSoundBuffer, &gameClock, hWnd);

    SGE_GameState *gameState = Win32_InitializeGameState();

    // Windows will clean this HDC up when the game exits
    HDC deviceContext = GetDC(hWnd);

    const int debugMessageMaxLength = 255;
    wchar_t debugMessage[debugMessageMaxLength];
    size_t debugMessageBufferSize = debugMessageMaxLength * sizeof(wchar_t);

    LARGE_INTEGER lastTime;
    QueryPerformanceCounter(&lastTime);
    while (g_IsApplicationRunning)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                g_IsApplicationRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        for (int i = 0; i < SGE_ControllerCount; i++)
        {
            gameState->Controllers[i].FinalizeState();
        }

        SGE_UpdateAndRender(gameState, (SGE_VideoBuffer *)&g_VideoBackBuffer);        
        Win32_OutputSoundSamples(&gameClock, gameState, win32SoundOutput, gameSoundBuffer);
        
        real32_t elapsedSeconds = Win32_GetElapsedSeconds(lastTime);

        if (elapsedSeconds > gameClock.TargetFrameLengthSeconds)
        {
            Debug_Log(Debug_Error, L"Missed a frame!");
        }

        while (elapsedSeconds < gameClock.TargetFrameLengthSeconds)
        {
            elapsedSeconds = Win32_GetElapsedSeconds(lastTime);
        }
        
        real64_t elapsedMilliseconds = elapsedSeconds * 1000.0;
        real64_t fps = 1.0 / elapsedSeconds;

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        lastTime = currentTime;

        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);
        Win32_PaintWindow(deviceContext, clientRect);        

        StringCbPrintfW(debugMessage, debugMessageBufferSize, L"%.02f ms - %.02f fps", elapsedMilliseconds, fps);
        Debug_Log(Debug_Verbose, debugMessage);
    }

    return 0;
}
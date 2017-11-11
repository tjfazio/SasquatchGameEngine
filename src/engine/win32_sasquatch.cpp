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
#include "ResourceCache.h"

using namespace Sasquatch;

namespace 
{
    using namespace Sasquatch::Platform;

    TCHAR szWindowClass[] = _T("sasquatch");
    TCHAR szTitle[] = _T("Sasquatch Game Engine");
    
    const uint64_t Win32_PlatformMemorySize = KILOBYTES(64);
    const uint64_t Win32_GameStaticMemorySize = GIGABYTES(2);
    
    typedef struct tagBitmapBuffer {
        int32_t Height;
        int32_t Width;
        int32_t Stride;
        void *Memory;
        BITMAPINFO Info;
        HBITMAP BitmapHandle;
        HDC DeviceContext;
    } Win32_BitmapBuffer;
    
    bool g_IsApplicationRunning;
    Win32_BitmapBuffer g_VideoBackBuffer;
    // HACK: remove global pointer after fixing input polling code
    GameState *g_GameState;
    Resources::ResourceCache g_ResourceCache;

    void Win32_ResizeWindow(
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
    
    void Win32_PaintWindow(
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
    
    void Win32_InitializeDefaultKeyboardMap(Input::Controller *keyboard)
    {
        keyboard->MapAction(Input::ActionCodes::Up, 'W', VK_UP);
        keyboard->MapAction(Input::ActionCodes::Down, 'S', VK_DOWN);
        keyboard->MapAction(Input::ActionCodes::Left, 'A', VK_LEFT);
        keyboard->MapAction(Input::ActionCodes::Right, 'D', VK_RIGHT);
        keyboard->MapAction(Input::ActionCodes::Action1, 'Q', VK_SPACE);
    }

    real32_t Win32_GetElapsedSeconds(LARGE_INTEGER lastTime)
    {
        LARGE_INTEGER counterFrequency;
        QueryPerformanceFrequency(&counterFrequency);
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        int64_t timeDelta = currentTime.QuadPart - lastTime.QuadPart;
        real32_t elapsedSeconds = ((real32_t)timeDelta) / counterFrequency.QuadPart;
        return elapsedSeconds;
    }

    void Win32_InitializeSound(
        Win32_SoundOutput **win32SoundOutputPtr, 
        SoundBuffer **gameSoundBufferPtr,
        GameClock *gameClock,
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

        SoundBuffer *gameSoundBuffer = (SoundBuffer *)(platformMemory + platformMemoryNextByte);
        platformMemoryNextByte += sizeof(SoundBuffer);

        gameSoundBuffer->SamplesPerSecond = Win32_SoundSamplesPerSecond;
        gameSoundBuffer->NumChannels = Win32_NumChannels;
        gameSoundBuffer->BufferSize = soundBufferSize;
        gameSoundBuffer->Memory = (sample_t *)(platformMemory + platformMemoryNextByte);
        platformMemoryNextByte += soundBufferSize;
        
        assert(Win32_PlatformMemorySize > platformMemoryNextByte);

        *win32SoundOutputPtr = win32SoundOutput;
        *gameSoundBufferPtr = gameSoundBuffer;
    }

    GameState *Win32_InitializeGameState()
    {
        uint8_t *gameStaticMemory = (uint8_t *)VirtualAlloc(NULL, Win32_GameStaticMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        assert(gameStaticMemory != NULL);
        assert(Win32_GameStaticMemorySize > sizeof(GameState));
        GameState *gameState = (GameState *)gameStaticMemory;
        // HACK: need a global pointer for updating the game controllers from Windows events 
        g_GameState = gameState;

        InitializeGameState(gameState);
        Win32_InitializeDefaultKeyboardMap(&gameState->Controllers[Keyboard]);

        return gameState;
    }
}

void Win32_HandleKey(uint32_t virtualKeyCode, bool isKeyDown)
{
    if (g_GameState == NULL)
    {
        return;
    }
    if (virtualKeyCode >= 0 && virtualKeyCode <= 0xFF)
    {
        g_GameState->Controllers[Keyboard].UpdateState((uint8_t)virtualKeyCode, isKeyDown);
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
    wcex.lpfnWndProc = (WNDPROC)WndProc;
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
    
    Debug::InitializeLog(Debug::Warning, "DebugLog.txt");
    if (!Platform::InitializeFileSystem())
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

    GameClock gameClock = {};
    
    // TODO: calculate these values based on hardware capabilities
    gameClock.TargetFrameRate = 30;
    gameClock.TargetFrameLengthSeconds = (real32_t)(1.0 / gameClock.TargetFrameRate);
    // TODO: figure out why I need so much padding for sound to work
    gameClock.FrameVariationSeconds = (real32_t)(2.0 * gameClock.TargetFrameLengthSeconds);

    Win32_SoundOutput *win32SoundOutput;
    SoundBuffer *gameSoundBuffer;
    Win32_InitializeSound(&win32SoundOutput, &gameSoundBuffer, &gameClock, hWnd);

    g_ResourceCache.Initialize(Resources::MaxResourceMemorySize);
    GameState *gameState = Win32_InitializeGameState();

    // Windows will clean this HDC up when the game exits
    HDC deviceContext = GetDC(hWnd);
    
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

        for (int i = 0; i < ControllerCount; i++)
        {
            gameState->Controllers[i].FinalizeState();
        }

        UpdateAndRender(gameState, (VideoBuffer *)&g_VideoBackBuffer);        
        Win32_OutputSoundSamples(&gameClock, gameState, win32SoundOutput, gameSoundBuffer);
        
        real32_t elapsedSeconds = Win32_GetElapsedSeconds(lastTime);

        if (elapsedSeconds > gameClock.TargetFrameLengthSeconds)
        {
            Debug::Log(Debug::Error, "Missed a frame!");
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

        // TODO: fix this mess
        const int debugMessageMaxLength = 1024;
        char debugMessage[debugMessageMaxLength];
        size_t debugMessageBufferSize = debugMessageMaxLength * sizeof(char);
        StringCbPrintfA(debugMessage, debugMessageBufferSize, "%.02f ms - %.02f fps", elapsedMilliseconds, fps);
        Debug::Log(Debug::Verbose, debugMessage);
    }

    return 0;
}
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>
#include <strsafe.h>
#include <dsound.h>
#include <assert.h>

#include "common.h"
#include "win32_util.h"

// Unity build ¯\_(ツ)_/¯
#include "input.cpp"
#include "sasquatch.cpp"

// function pointers for DLLs
typedef HRESULT (*DirectSoundCreate_T)(LPCGUID lpcGuidDevice, LPDIRECTSOUND * ppDS, LPUNKNOWN pUnkOuter);

global_variable TCHAR szWindowClass[] = _T("sasquatch");
global_variable TCHAR szTitle[] = _T("Sasquatch Game Engine");

const int32_t Win32_SoundSamplesPerSecond = 44100;
const int32_t Win32_NumChannels = 2;
const int32_t Win32_SoundBufferSize = 2 * Win32_NumChannels * sizeof(sample_t) * Win32_SoundSamplesPerSecond;

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

typedef struct tagSoundOutput {
    int32_t SoundBufferSize;
    int32_t Cursor;
    int32_t LatencySampleCount;
    bool IsSoundValid;
    LPDIRECTSOUNDBUFFER PrimaryBuffer;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
} Win32_SoundBuffer;
} Win32_SoundOutput;

global_variable bool g_IsApplicationRunning;
global_variable Win32_BitmapBuffer g_VideoBackBuffer;
global_variable Win32_SoundOutput g_Win32SoundOutput;

global_variable SGE_GameState g_GameState;
global_variable SGE_SoundBuffer g_SoundBuffer;

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
    
    g_GameState.Keyboard.ClearState();
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
    if (virtualKeyCode >= 0 && virtualKeyCode <= 0xFF)
    {
        g_GameState.Keyboard.SetState((uint8_t)virtualKeyCode, isKeyDown);
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

internal void Win32_InitializeDirectSound(
    HWND hwnd,
    int32_t soundBufferSize,
    int32_t samplesPerSecond)
{
    HMODULE directSoundLib = LoadLibrary("dsound.dll");
    if (!directSoundLib)
    { 
        // log error
        return;
    }

    DirectSoundCreate_T directSoundCreate = (DirectSoundCreate_T)GetProcAddress(directSoundLib, "DirectSoundCreate");
    LPDIRECTSOUND directSound;
    if (!directSoundCreate || !SUCCEEDED(directSoundCreate(NULL, &directSound, NULL)))
    {
        // log error
        return;
    }
    if (!SUCCEEDED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)))
    {
        // log error
        return;
    }

    DSBUFFERDESC primaryBufferDescription = {};
    primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
    primaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    primaryBufferDescription.dwBufferBytes = 0;
    primaryBufferDescription.dwReserved = 0;
    primaryBufferDescription.lpwfxFormat = NULL;
    primaryBufferDescription.guid3DAlgorithm = GUID_NULL;
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDescription, &g_Win32SoundOutput.PrimaryBuffer, NULL)))
    {
        // log error
        return;
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = Win32_NumChannels;
    waveFormat.nSamplesPerSec = samplesPerSecond;
    waveFormat.wBitsPerSample = sizeof(sample_t) * 8;
    waveFormat.nBlockAlign = waveFormat.nChannels * sizeof(sample_t);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;
    if (!SUCCEEDED(g_Win32SoundOutput.PrimaryBuffer->SetFormat(&waveFormat)))
    {
        // log error
        return;
    }

    DSBUFFERDESC secondaryBufferDescription;
    secondaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
    secondaryBufferDescription.dwFlags = 0;
    secondaryBufferDescription.dwBufferBytes = soundBufferSize;
    secondaryBufferDescription.dwReserved = 0;
    secondaryBufferDescription.lpwfxFormat = &waveFormat;
    secondaryBufferDescription.guid3DAlgorithm = GUID_NULL;
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDescription, &g_Win32SoundOutput.SecondaryBuffer, NULL)))
    {
        // log error
        return;
    }
    g_Win32SoundOutput.SoundBufferSize = soundBufferSize;
}

internal void Win32_ClearSoundBuffer(Win32_SoundOutput *soundBuffer)
{
    void *soundBufferSection1;
    DWORD soundBytes1;
    void *soundBufferSection2;
    DWORD soundBytes2;
    HRESULT lockResult = soundBuffer->SecondaryBuffer->Lock(
                0, soundBuffer->SoundBufferSize,
                &soundBufferSection1, &soundBytes1, 
                &soundBufferSection2, &soundBytes2,
                0);
    if (SUCCEEDED(lockResult))
    {
        assert(soundBytes2 == 0);
        for (int i = 0; i < soundBytes1; i++)
        {
            (*(uint8_t*)soundBufferSection1) = 0;
        }
        HRESULT unlockResult =
            g_Win32SoundOutput.SecondaryBuffer->Unlock(
                soundBufferSection1, soundBytes1,
                soundBufferSection2, soundBytes2);
        if (!SUCCEEDED(unlockResult))
        {
            // log
        }
    }
}

internal void Win32_InitializeDefaultKeyboardMap()
{
    g_GameState.Keyboard.MapAction(SGE_Up, 'W', VK_UP);
    g_GameState.Keyboard.MapAction(SGE_Down, 'S', VK_DOWN);
    g_GameState.Keyboard.MapAction(SGE_Left, 'A', VK_LEFT);
    g_GameState.Keyboard.MapAction(SGE_Right, 'D', VK_RIGHT);
    g_GameState.Keyboard.MapAction(SGE_Action1, 'Q', VK_SPACE);
}

const uint32_t Win32_WaveFreq = 440;
const uint32_t Win32_Wavelength = Win32_SoundSamplesPerSecond / Win32_WaveFreq;

internal void Win32_OutputSoundSamples(
    SGE_GameClock *gameClock,
    SGE_GameState *gameState,
    Win32_SoundOutput *soundOutput, 
    SGE_SoundBuffer *gameSoundBuffer)
{
    const int debugMessageMaxLength = 255;
    wchar_t debugMessage[debugMessageMaxLength];
    size_t debugMessageBufferSize = debugMessageMaxLength * sizeof(wchar_t);

    DWORD playCursor;
    DWORD writeCursor;
    if(SUCCEEDED(soundOutput->SecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
    {
        if (!soundOutput->IsSoundValid)
        {
            OutputDebugStringW(L"Resetting write cursor\n");
            soundOutput->Cursor = writeCursor;
        } 
        StringCbPrintfW(debugMessage, debugMessageBufferSize, L"Play:%d\tWrite:%d\tCursor:%d\n", playCursor, writeCursor, soundOutput->Cursor);
        OutputDebugStringW(debugMessage);

        real32_t soundSeconds = gameClock->TargetFrameLengthSeconds + gameClock->FrameVariationSeconds;

        int32_t frameSoundBytes = (int32_t)(soundSeconds 
            * gameSoundBuffer->SamplesPerSecond 
            * sizeof(sample_t) 
            * gameSoundBuffer->NumChannels);
        int32_t targetCursor = playCursor + frameSoundBytes;
        if (targetCursor > soundOutput->SoundBufferSize)
        {
            targetCursor -= soundOutput->SoundBufferSize;
        }
        int32_t previousCursor = soundOutput->Cursor;
        int32_t bytesToWrite = targetCursor - previousCursor;
        if (bytesToWrite == 0)
        {
            OutputDebugStringW(L"Play cursor hasn't advanced since last frame. Strange.");
            return;
        }
        if (bytesToWrite < 0)
        {
            // looped around
            bytesToWrite += soundOutput->SoundBufferSize;
        }
        
        if (bytesToWrite > gameSoundBuffer->BufferSize)
        {
            OutputDebugStringW(L"Sound lagging behind, not enough space in buffer to catch up");
            bytesToWrite = gameSoundBuffer->BufferSize;
        }

        StringCbPrintfW(debugMessage, debugMessageBufferSize, L"Sound bytes to write: %d\n", bytesToWrite);
        OutputDebugStringW(debugMessage);

        assert(bytesToWrite % (gameSoundBuffer->NumChannels * sizeof(sample_t)) == 0);
        gameSoundBuffer->SampleCount = bytesToWrite / (gameSoundBuffer->NumChannels * sizeof(sample_t));

        SGE_GetSoundSamples(gameState, gameSoundBuffer);

        void *soundBufferSection1;
        DWORD soundBytes1;
        void *soundBufferSection2;
        DWORD soundBytes2;
        HRESULT lockResult = soundOutput->SecondaryBuffer->Lock(
            soundOutput->Cursor, bytesToWrite,
            &soundBufferSection1, &soundBytes1, 
            &soundBufferSection2, &soundBytes2,
            0);
        if(SUCCEEDED(lockResult))
        {
            int32_t sampleIndex = 0;
            int32_t section1SampleCount = soundBytes1 / sizeof(sample_t);
            sample_t *section1 = (sample_t *)soundBufferSection1;
            for (int i = 0; i < section1SampleCount; i++)
            {
                section1[i] = gameSoundBuffer->Memory[sampleIndex];
                sampleIndex++;
            }

            sample_t *section2 = (sample_t *)soundBufferSection2;
            int32_t section2SampleCount = soundBytes2 / sizeof(sample_t);
            for (int i = 0; i < section2SampleCount; i++)
            {
                section2[i] = gameSoundBuffer->Memory[sampleIndex];
                sampleIndex++;
            }

            soundOutput->Cursor = targetCursor;

            if (!SUCCEEDED(soundOutput->SecondaryBuffer->Unlock(
                    soundBufferSection1, soundBytes1,
                    soundBufferSection2, soundBytes2)))
            {                    
                OutputDebugStringW(L"Sound bytes unlock failed\n");
                soundOutput->IsSoundValid = false;
            }
            else
            {
                soundOutput->IsSoundValid = true;
            }
        }
        else
        {                    
            OutputDebugStringW(L"Sound bytes lock failed\n");
            soundOutput->IsSoundValid = false;
        }
    }
    else
    {
        OutputDebugStringW(L"Get sound cursors failed\n");
        soundOutput->IsSoundValid = false;
    }
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

const uint64_t Win32_PlatformMemorySize = KILOBYTES(64);
global_variable void *g_PlatformMemory;

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

    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
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
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);    

    g_IsApplicationRunning = true;

    SGE_GameClock gameClock = {};
    
    // TODO: calculate these values based on hardware capabilities
    gameClock.TargetFrameRate = 30;
    gameClock.TargetFrameLengthSeconds = 1.0 / gameClock.TargetFrameRate;
    gameClock.FrameVariationSeconds = 2.0 * gameClock.TargetFrameLengthSeconds;
    g_PlatformMemory = VirtualAlloc(NULL, Win32_PlatformMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Win32_InitializeDirectSound(hWnd, Win32_SoundBufferSize, Win32_SoundSamplesPerSecond);
    Win32_ClearSoundBuffer(&g_Win32SoundOutput);
    g_Win32SoundOutput.SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    int32_t samplesPerFrame = Win32_SoundSamplesPerSecond / gameClock.TargetFrameRate;
    uint64_t soundBufferSize = 4 * Win32_NumChannels * sizeof(sample_t) * samplesPerFrame;    
    g_Win32SoundOutput.LatencySampleCount = 4 * samplesPerFrame;
    assert(Win32_PlatformMemorySize > soundBufferSize);

    g_SoundBuffer.SamplesPerSecond = Win32_SoundSamplesPerSecond;
    g_SoundBuffer.NumChannels = Win32_NumChannels;
    g_SoundBuffer.Memory = (sample_t *)g_PlatformMemory;
    g_SoundBuffer.BufferSize = soundBufferSize;

    Win32_InitializeDefaultKeyboardMap();
    SGE_Init(&g_GameState);

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

        SGE_UpdateAndRender(&g_GameState, (SGE_VideoBuffer *)&g_VideoBackBuffer);        
        Win32_OutputSoundSamples(&gameClock, &g_GameState, &g_Win32SoundOutput, &g_SoundBuffer);
        
        real32_t elapsedSeconds = Win32_GetElapsedSeconds(lastTime);

        if (elapsedSeconds > gameClock.TargetFrameLengthSeconds)
        {
            OutputDebugStringW(L"Missed a frame!\n");
        }

        while (elapsedSeconds < gameClock.TargetFrameLengthSeconds)
        {
            elapsedSeconds = Win32_GetElapsedSeconds(lastTime);
        }
        
        real32_t elapsedMilliseconds = elapsedSeconds * 1000.0;
        real32_t fps = 1.0 / elapsedSeconds;

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        lastTime = currentTime;

        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);
        Win32_PaintWindow(deviceContext, clientRect);        

        StringCbPrintfW(debugMessage, debugMessageBufferSize, L"%.02f ms - %.02f fps \n", elapsedMilliseconds, fps);
        OutputDebugStringW(debugMessage);
    }

    return 0;
}
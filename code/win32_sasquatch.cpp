#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>
#include <strsafe.h>
#include <dsound.h>

#include "win32_util.h"

// Unity build ¯\_(ツ)_/¯
#include "input.cpp"
#include "sasquatch.cpp"

#define global_variable static
#define internal_function static
#define local_persist static

// function pointers for DLLs
typedef HRESULT (*DirectSoundCreate_T)(LPCGUID lpcGuidDevice, LPDIRECTSOUND * ppDS, LPUNKNOWN pUnkOuter);

global_variable TCHAR szWindowClass[] = _T("sasquatch");
global_variable TCHAR szTitle[] = _T("Sasquatch Game Engine");

const int32_t Win32_SoundSamplesPerSecond = 44100;
const int32_t Win32_SoundBufferSize = 2 * 4 * Win32_SoundSamplesPerSecond;

typedef struct tagBitmapBuffer {
    int32_t Height;
    int32_t Width;
    void *Memory;
    BITMAPINFO Info;
    HBITMAP BitmapHandle;
    HDC DeviceContext;
} Win32_BitmapBuffer;

typedef struct tagSoundBuffer {
    uint32_t SecondaryBufferSize;
    LPDIRECTSOUNDBUFFER PrimaryBuffer;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
} Win32_SoundBuffer;

global_variable bool g_IsApplicationRunning;
global_variable Win32_BitmapBuffer g_VideoBackBuffer;
global_variable Win32_SoundBuffer g_SoundBuffer;

global_variable SGE_GameState g_GameState;

internal_function void Win32_ResizeWindow(
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
        // TODO(tjfazio) - mess with this for multiple monitors??
        g_VideoBackBuffer.DeviceContext = CreateCompatibleDC(targetDeviceContext);
    }

    g_VideoBackBuffer.Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_VideoBackBuffer.Info.bmiHeader.biWidth = Win32_GetRectWidth(clientRect);
    g_VideoBackBuffer.Width = g_VideoBackBuffer.Info.bmiHeader.biWidth;
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

internal_function void Win32_PaintWindow(
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

internal_function void Win32_InitializeDirectSound(
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
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDescription, &g_SoundBuffer.PrimaryBuffer, NULL)))
    {
        // log error
        return;
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = samplesPerSecond;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = (waveFormat.nSamplesPerSec * waveFormat.nBlockAlign);
    waveFormat.cbSize = 0;
    if (!SUCCEEDED(g_SoundBuffer.PrimaryBuffer->SetFormat(&waveFormat)))
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
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDescription, &g_SoundBuffer.SecondaryBuffer, NULL)))
    {
        // log error
        return;
    }
    g_SoundBuffer.SecondaryBufferSize = soundBufferSize;
}

internal_function void Win32_InitializeDefaultKeyboardMap()
{
    g_GameState.Keyboard.MapAction(SGE_Up, 'W', VK_UP);
    g_GameState.Keyboard.MapAction(SGE_Down, 'S', VK_DOWN);
    g_GameState.Keyboard.MapAction(SGE_Left, 'A', VK_LEFT);
    g_GameState.Keyboard.MapAction(SGE_Right, 'D', VK_RIGHT);
}

const uint32_t Win32_WaveFreq = 440;
const uint32_t Win32_Wavelength = Win32_SoundSamplesPerSecond / Win32_WaveFreq; 

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

    Win32_InitializeDirectSound(hWnd, Win32_SoundBufferSize, Win32_SoundSamplesPerSecond);
    Win32_InitializeDefaultKeyboardMap();

    g_IsApplicationRunning = true;
    bool soundStarted = false;
    SGE_Init(&g_GameState);

    // Windows will clean this HDC up when the game exits
    HDC deviceContext = GetDC(hWnd);

    const int perfCounterMessageMaxLength = 255;
    wchar_t perfCounterMessage[perfCounterMessageMaxLength];
    size_t perfCounterMessageBufferSize = perfCounterMessageMaxLength * sizeof(wchar_t);

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);

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

        DWORD playCursor;
        DWORD writeCursor;
        if(!soundStarted)
        {
            if(SUCCEEDED(g_SoundBuffer.SecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                uint32_t bytesToLock;
                if (playCursor > writeCursor)
                {
                    bytesToLock = playCursor - writeCursor;
                }
                else
                {
                    bytesToLock = g_SoundBuffer.SecondaryBufferSize - writeCursor + playCursor;
                }

                void *soundBufferSection1;
                DWORD soundBytes1;
                void *soundBufferSection2;
                DWORD soundBytes2;
                DWORD samplePosition = writeCursor;
                HRESULT lockResult = g_SoundBuffer.SecondaryBuffer->Lock(
                    writeCursor, bytesToLock, 
                    &soundBufferSection1, &soundBytes1, 
                    &soundBufferSection2, &soundBytes2,
                    0);
                if(SUCCEEDED(lockResult))
                {
                    sample_t *section1 = (sample_t *)soundBufferSection1;
                    sample_t amp = 5000;
                    for (int i = 0; i < soundBytes1; i += sizeof(sample_t))
                    {
                        // flip signal every 2 * half_wavelength [L'R'L'R'L,R,L,R,]
                        uint32_t sampleIndex = samplePosition / Win32_Wavelength;
                        sample_t sample = ((sampleIndex % 2) == 0) ? amp : -amp;
                        *(section1++) = sample;
                        samplePosition = samplePosition + 1;
                        if (samplePosition >= g_SoundBuffer.SecondaryBufferSize) samplePosition = 0;
                    }

                    sample_t *section2 = (sample_t *)soundBufferSection2;
                    for (int i = 0; i < soundBytes2; i += sizeof(sample_t))
                    {
                        // flip signal every 2 * half_wavelength [L'R'L'R'L,R,L,R,]
                        uint32_t sampleIndex = samplePosition / Win32_Wavelength;
                        sample_t sample = ((sampleIndex % 2) == 0) ? amp : -amp;
                        *(section2++) = sample;
                        samplePosition = samplePosition + 1;
                        if (samplePosition >= g_SoundBuffer.SecondaryBufferSize) samplePosition = 0;
                    }

                    if (!SUCCEEDED(g_SoundBuffer.SecondaryBuffer->Unlock(
                            soundBufferSection1, soundBytes1,
                            soundBufferSection2, soundBytes2)))
                    {
                        // log error - I don't think we can do much in this case
                    }

                    if (!soundStarted)
                    {
                        g_SoundBuffer.SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                        soundStarted = true;
                    }
                }
            }
        }
        
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        Win32_PaintWindow(deviceContext, clientRect);

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        int64_t timeDelta = currentTime.QuadPart - lastTime.QuadPart;
        double elapsedMilliseconds = (timeDelta * 1000.0) / counterFrequency.QuadPart;
        double fps = 1000.0 / elapsedMilliseconds;

        StringCbPrintfW(perfCounterMessage, perfCounterMessageBufferSize, L"%.02f ms - %.02f fps \n", elapsedMilliseconds, fps);
        OutputDebugStringW(perfCounterMessage);

        lastTime = currentTime;
    }

    return 0;
}
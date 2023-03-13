#include <Windows.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define VK_Q_KEY 0x51
#define QUIT_APPLICATION_KEY_CODE VK_Q_KEY

static TCHAR szWindowClass[] = _T("windows_class_name");
static TCHAR szTitle[] = _T("windows_title");

void mHandleResizeMessage( //
    _In_ HWND hwnd,        //
    _In_ UINT uMsg,        //
    _In_ WPARAM wParam,    //
    _In_ LPARAM lParam     //
)
{
    InvalidateRect(hwnd, NULL, FALSE);
}

void mHandlePaintMessage( //
    _In_ HWND hwnd,       //
    _In_ UINT uMsg,       //
    _In_ WPARAM wParam,   //
    _In_ LPARAM lParam    //
)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(hwnd, &rc);

    if (rc.bottom == 0)
    {
        return;
    }

    hdc = BeginPaint(hwnd, &ps);

    // draw black background
    HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rc, bgBrush);
    EndPaint(hwnd, &ps);
}

void mHandleKeyUpMessage( //
    _In_ HWND hwnd,       //
    _In_ UINT uMsg,       //
    _In_ WPARAM wParam,   //
    _In_ LPARAM lParam    //
)
{
    int virtual_key_code = (int)wParam;
    if (virtual_key_code == QUIT_APPLICATION_KEY_CODE)
    {
        PostQuitMessage(0);
    }
}

LRESULT CALLBACK WndProc( //
    _In_ HWND hwnd,       //
    _In_ UINT uMsg,       //
    _In_ WPARAM wParam,   //
    _In_ LPARAM lParam    //
)
{
    if (uMsg == WM_PAINT)
    {
        mHandlePaintMessage(hwnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_SIZE)
    {
        mHandleResizeMessage(hwnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_KEYUP)
    {
        mHandleKeyUpMessage(hwnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

LARGE_INTEGER PERFORMANCE_COUNTER_FREQUENCY = {0};
LARGE_INTEGER GLOBAL_LEFT_CLICK_LAST_DOWN_COUNT = {0};

LRESULT CALLBACK mouse_event_monitor_hook_proc( //
    _In_ int nCode,                             //
    _In_ WPARAM wParam,                         //
    _In_ LPARAM lParam                          //
)
{
    if (nCode < 0)
    {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    else
    {
        // monitor mouse event for double clicks and block them
        if (PERFORMANCE_COUNTER_FREQUENCY.QuadPart != 0)
        {
            if (wParam == WM_LBUTTONDOWN)
            {
                LARGE_INTEGER current_count;
                QueryPerformanceCounter(&current_count);
                if (GLOBAL_LEFT_CLICK_LAST_DOWN_COUNT.QuadPart != 0)
                {
                    double delta_count = (double)(current_count.QuadPart - GLOBAL_LEFT_CLICK_LAST_DOWN_COUNT.QuadPart);
                    double delta_seconds = (double)delta_count / (double)PERFORMANCE_COUNTER_FREQUENCY.QuadPart;
                    int delta_milliseconds = (int)(delta_seconds * 1000);
                    if (delta_milliseconds < 100)
                    {
                        printf("double click detected %d\n", delta_milliseconds);
                        return -1;
                    }
                    else
                    {
                        printf("mouse down %d\n", delta_milliseconds);
                    }
                }

                GLOBAL_LEFT_CLICK_LAST_DOWN_COUNT = current_count;
            }
        }

        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
}

int CALLBACK wWinMain(                //
    _In_ HINSTANCE hInstance,         //
    _In_opt_ HINSTANCE hPrevInstance, //
    _In_ LPWSTR lpCmdLine,            //
    _In_ int nCmdShow                 //
)
{
    // default window width and height values
    int nWidth = 720;
    int nHeight = 480;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(0);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        printf("RegisterClassEx failed at %s:%d\n", __FILE__, __LINE__);
        return -1;
    }

    HOOKPROC low_level_mouse_hook_proc = mouse_event_monitor_hook_proc;
    HHOOK mouse_hook_handle = SetWindowsHookEx( //
        WH_MOUSE_LL,                            //
        low_level_mouse_hook_proc,              //
        NULL,                                   //
        0                                       //
    );

    if (mouse_hook_handle == NULL)
    {
        printf("SetWindowsHookEx failed at %s:%d\n", __FILE__, __LINE__);
        return -1;
    }

    HWND hwnd = CreateWindow(                //
        szWindowClass,                       //
        szTitle,                             //
        WS_OVERLAPPEDWINDOW | WS_EX_LAYERED, //
        CW_USEDEFAULT,                       //
        CW_USEDEFAULT,                       //
        nWidth,                              //
        nHeight,                             //
        NULL,                                //
        NULL,                                //
        hInstance,                           //
        NULL                                 //
    );

    if (!hwnd)
    {
        printf("CreateWindow failed at %s:%d\n", __FILE__, __LINE__);
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(mouse_hook_handle);

    return (int)msg.wParam;
}

int main()
{
    LARGE_INTEGER performance_counter_frequency;
    QueryPerformanceFrequency(&performance_counter_frequency);
    PERFORMANCE_COUNTER_FREQUENCY = performance_counter_frequency;

    return wWinMain(           //
        GetModuleHandle(NULL), //
        NULL,                  //
        GetCommandLine(),      //
        SW_SHOWNORMAL          //
    );
};

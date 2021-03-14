#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shellapi.h>
#include <iostream>
#include <urlmon.h>
#include <tchar.h>
#include <d3d9.h>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <limits.h>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "GH Injector.h"
#include <TlHelp32.h>

#define WINDOW_WIDTH 190
#define WINDOW_HEIGHT 200

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT     
#define NK_IMPLEMENTATION
#define NK_D3D9_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_d3d9.h"

using namespace std;

/*#define INCLUDE_ALL */
/*#define INCLUDE_STYLE */
/*#define INCLUDE_CALCULATOR */
/*#define INCLUDE_OVERVIEW */
/*#define INCLUDE_NODE_EDITOR */

#ifdef INCLUDE_ALL
#define INCLUDE_STYLE
#define INCLUDE_CALCULATOR
#define INCLUDE_OVERVIEW
#define INCLUDE_NODE_EDITOR
#endif

enum theme { THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK };

static void
set_style(struct nk_context* ctx, enum theme theme)
{
    struct nk_color table[NK_COLOR_COUNT];
    if (theme == THEME_DARK) {
        table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
        table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
        table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
        table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
        table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
        table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
        table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
        table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
        table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
        table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
        table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
        table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
        table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
        nk_style_from_table(ctx, table);
    }
    else {
        nk_style_default(ctx);
    }
}


static IDirect3DDevice9* device;
static IDirect3DDevice9Ex* deviceEx;
static D3DPRESENT_PARAMETERS present;

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (device)
        {
            UINT width = LOWORD(lparam);
            UINT height = HIWORD(lparam);
            if (width != 0 && height != 0 &&
                (width != present.BackBufferWidth || height != present.BackBufferHeight))
            {
                nk_d3d9_release();
                present.BackBufferWidth = width;
                present.BackBufferHeight = height;
                HRESULT hr = IDirect3DDevice9_Reset(device, &present);
                NK_ASSERT(SUCCEEDED(hr));
                nk_d3d9_resize(width, height);
            }
        }
        break;
    }

    if (nk_d3d9_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void create_d3d9_device(HWND wnd)
{
    HRESULT hr;

    present.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    present.BackBufferWidth = WINDOW_WIDTH;
    present.BackBufferHeight = WINDOW_HEIGHT;
    present.BackBufferFormat = D3DFMT_X8R8G8B8;
    present.BackBufferCount = 1;
    present.MultiSampleType = D3DMULTISAMPLE_NONE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.hDeviceWindow = wnd;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D24S8;
    present.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    present.Windowed = TRUE;


    {
        typedef HRESULT WINAPI Direct3DCreate9ExPtr(UINT, IDirect3D9Ex**);
        Direct3DCreate9ExPtr* Direct3DCreate9Ex = (Direct3DCreate9ExPtr*)GetProcAddress(GetModuleHandleA("d3d9.dll"), "Direct3DCreate9Ex");

        if (Direct3DCreate9Ex) {
            IDirect3D9Ex* d3d9ex;
            if (SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex))) {
                hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                    D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                    &present, NULL, &deviceEx);
                if (SUCCEEDED(hr)) {
                    device = (IDirect3DDevice9*)deviceEx;
                }
                else {
                    /* hardware vertex processing not supported, no big deal
                    retry with software vertex processing */
                    hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                        &present, NULL, &deviceEx);
                    if (SUCCEEDED(hr)) {
                        device = (IDirect3DDevice9*)deviceEx;
                    }
                }
                IDirect3D9Ex_Release(d3d9ex);
            }
        }
    }

    if (!device) {
        IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
            &present, &device);
        if (FAILED(hr)) {
            /* hardware vertex processing not supported, no big deal
            retry with software vertex processing */
            hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                &present, &device);
            NK_ASSERT(SUCCEEDED(hr));
        }
        IDirect3D9_Release(d3d9);
    }
}

const char szFilePath[] = "test_.dll";

DWORD FindProcessId(const char* szProcessName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    DWORD FoundProcessId = 0;

    while (Process32Next(hSnapshot, &pe32))
    {
        if (!strcmp(szProcessName, pe32.szExeFile))
        {
            FoundProcessId = pe32.th32ProcessID;
            break;
        }
    }

    CloseHandle(hSnapshot);
    return FoundProcessId;
}


int main(void)
{
    HANDLE ESP = NULL;
    struct nk_context* ctx;
    struct nk_colorf bg;

    WNDCLASSW wc;
    RECT rect = { -7, -37, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    int show_overview = nk_true;
    int show_esp = nk_false;
    int show_overview_esp = nk_true;
    int show_bhop = nk_false;
    int running = 1;
    bool BunnyOn = false;
    char buf[256] = { 0 };
    char* df = "df";
    /* char * d = wwe;*/
    char buff[256] = { 0 };
    static int lay = 4;
    // const char* update = "";


    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NuklearWindowClass";
    RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Loader",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);

    create_d3d9_device(wnd);


    ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);

    {struct nk_font_atlas* atlas;
    nk_d3d9_font_stash_begin(&atlas);
    struct nk_font_config cfg = nk_font_config(15);
    cfg.range = nk_font_cyrillic_glyph_ranges();
    cfg.merge_mode = nk_false;
    cfg.coord_type = NK_COORD_UV;
    cfg.oversample_h = (7, 7);
    cfg.oversample_v = (7, 7);
    cfg.pixel_snap = 100;
    cfg.spacing = nk_vec2(25, 25);
    struct nk_font* clean = nk_font_atlas_add_from_file(atlas, "C:/Users/Azboka_2/Desktop/Nuklear/master/extra_font/Roboto-Regular.ttf", 15, &cfg);
    nk_d3d9_font_stash_end();
    nk_style_set_font(ctx, &clean->handle);

    nk_d3d9_font_stash_end();
    ; }

    set_style(ctx, THEME_DARK);

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    strcpy(buf, "***");
    strcpy(buff, "***");

    while (running)
    {
        MSG msg;
        nk_input_begin(ctx);

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        nk_input_end(ctx);

        if (nk_begin(ctx, "Show", nk_rect(-3, -2, 210, 250),
            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE)) //  NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE))
        {
            enum { EASY, HARD };
            static int op = EASY;
            static int delay = 1;
            static int checkbox;
            static int check = 1;
 
            Sleep(1);

            /* nk_layout_row_static(ctx, 30, 80, 5);*/
             //nk_label(ctx, "ESP", NK_TEXT_LEFT);
             //if (show_esp)
             //    if (nk_button_label(ctx, u8"Кнопка")) {
             //        show_overview_esp = !show_overview_esp;
             //        show_esp = !show_esp;
             //        // MessageBox(NULL, "on", "Hello from Message Box", MB_OK);
             //        TerminateThread(ESP, 0);
             //        CloseHandle(ESP);
             //    }

             //if (show_overview_esp)
             //    if (nk_button_label(ctx, "Кнопка")) { //   if (nk_button_label(ctx, u8"Кнопка")) {
             //        show_overview_esp = !show_overview_esp;
             //        show_esp = !show_esp;
             //      /*  ESP = CreateThread(NULL, NULL, &virtualesp::esp_thread, NULL, NULL, NULL);*/
             //        // MessageBox(NULL, "off", "Hello from Message Box", MB_OK);
             //    }

             //nk_layout_row_static(ctx, 16, 120, 1);
             //if (nk_checkbox_label(ctx, "Show Friends", &check)) {
             //    if (check == 1) {
             //        sleep_time = 10;
             //    }
             //    if (check == 0) {
             //        sleep_time = 8;
             //    }
             //}

         /*    nk_layout_row_static(ctx, 0, 80, 25);*/


            //if (show_bhop)
            //    nk_layout_row_static(ctx, 30, 80, 5);
            //if (show_bhop)
            //    nk_label(ctx, "DLL:", NK_TEXT_LEFT);
            //if (show_bhop)
            //    if (nk_button_label(ctx, "Download")) {
            //        /*show_overview = !show_overview;
            //        show_bhop = !show_bhop;*/
            //         MessageBox(NULL, "on", "Hello from Message Box", MB_OK);
            //        /*BunnyOn = !BunnyOn;*/
            //        /*sleep_time = 30;*/
            //    }

            if (show_bhop)
                nk_layout_row_static(ctx, 30, 80, 5);
            if (show_bhop)
                nk_label(ctx, "Inject:", NK_TEXT_LEFT);
            if (show_bhop)
                nk_layout_row_static(ctx, 30, 60, 50);
                if (nk_button_label(ctx, "On")) {
                    HMODULE hGHInjector = LoadLibraryA("gh_injector.dll");
                    fnInject Inject;
                    Inject = (fnInject)GetProcAddress(hGHInjector, "InjectA");

                    INJECTIONDATA data;
                    ZeroMemory(&data, sizeof(INJECTIONDATA));
                    data.Mode = 2;
                    data.Method = 0;
                    data.Flags = INJ_ERASE_HEADER | INJ_SHIFT_MODULE | INJ_UNLINK_FROM_PEB;
                    data.ProcessID = FindProcessId("test.exe");
                    /*strcpy(data.szDllPath, szFilePath);*/
                    /*memcpy(data.szDllPath, szFilePath, strlen(szFilePath) + 1);*/

                    RtlMoveMemory(data.szDllPath, szFilePath, strlen(szFilePath) + 1);
                    Inject(&data);
                    FreeLibrary(hGHInjector);

                    MessageBox(NULL, "on", "Injection", MB_OK);
                    /*show_overview = !show_overview;
                    show_bhop = !show_bhop;*/
                    // MessageBox(NULL, "on", "Hello from Message Box", MB_OK);
                    /*BunnyOn = !BunnyOn;*/
                    /*sleep_time = 30;*/
                }

            /* nk_slider_int(ctx, 1, &delay, 100, 1);*/
            nk_layout_row_static(ctx, 20, 60, 25);
            if (show_overview)
                nk_label(ctx, "Username", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 20, 2);
            if (show_overview)
                nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, sizeof(buf) - 1, nk_filter_default);

            nk_layout_row_static(ctx, 20, 60, 25);
            if (show_overview)
                nk_label(ctx, "Password", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 20, 2);
            if (show_overview)
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buff, sizeof(buff) - 1, nk_filter_default);
            nk_layout_row_static(ctx, 10, 10, 10);

            if (show_overview)
                nk_layout_row_static(ctx, 30, 80, 50);
            if (show_overview)
                if (nk_button_label(ctx, "Login")) {
                    if (!strcmp(buff, "123") && (!strcmp(buf, "123"))) {
                        show_overview = !show_overview;
                        show_bhop = !show_bhop;
             //           AllocConsole();
             //           SetConsoleTitle("Download...");
             //           freopen("CONOUT$", "w", stdout);
             //           freopen("CONIN$", "r", stdin);

             ///*           DownloadProgress progress;*/
             //       /*    HRESULT hr = URLDownloadToFileA(0,
             //               "https://drive.google.com/uc?id=1TQsHm2N7HQxm7jXt3hr85kSxnzCdzzUW&authuser=0&export=download",
             //               "test_.dll", 0,
             //               static_cast<IBindStatusCallback*>(&progress));*/

             //           FreeConsole();

                    }
                    else
                        MessageBoxW(NULL, L"Incorrect password or username entered", L"Loader.exe", MB_OK);
                }

            /* nk_layout_row_dynamic(ctx, 30, 2);
             if (nk_option_label(ctx, "easy", op == EASY)) op = EASY; {*/

             /* nk_layout_row_static(ctx, 30, 130, 1);
                nk_label(ctx, "risk", NK_TEXT_LEFT);
              if (nk_button_label(ctx, "Check for update")) {
                  //MessageBoxW(NULL, L"Нет доступных обновлений", L"Событие", MB_OK);
              }*/

              /* nk_property_int(ctx, "Bhop delay", 1, &delay, 100, 3, 1);*/

               /*nk_layout_row_dynamic(ctx, 14, 1);
               nk_label(ctx, "Bhop delay", NK_TEXT_LEFT);*/
               /*  nk_slider_int(ctx, 1, &delay, 100, 1);*/

                 //if (delay <= 5) {
                 //    sizeof(delay);
                 //    int lay = 5; // переопределить переменную переменная
                 //}

                 //if (delay = 1) {
                 //    sizeof(delay);
                 //    int lay = 5; // переопределить переменную переменная
                 //}

              /*   nk_slider_int(ctx, 1, &delay, 100, 1);*/

                 /* nk_label(ctx, "Enter your license key", NK_TEXT_LEFT);
                  nk_layout_row_dynamic(ctx, 20, 2);
                  nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, sizeof(buf) - 1, nk_filter_default);
                  if (nk_button_label(ctx, "Done")) {
                      std::cout << buf << std::endl;
                      MessageBoxW(NULL, buf, _T("строчка из файла"),  MB_OK);
                  }*/
                  /* nk_layout_row_dynamic(ctx, 30, 2);
                   if (nk_option_label(ctx, "easy", op == EASY)) op = EASY; {

                   }
                   if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
                   if (nk_option_label(ctx, "normal", op == NORMAL)) op = NORMAL; {
                       if (op == 2) {
                           MessageBoxW(NULL, L"Нет доступных", L"Событие", MB_OK);
                       }
                   }*/


                   //nk_layout_row_dynamic(ctx, 15, 5);
                   ///* nk_label(ctx, "2020 All Rights Reserved", NK_TEXT_LEFT);*/
                   //nk_layout_row_static(ctx, 25, 55, 50);
                   //if (nk_button_label(ctx, "vk.com/bunnyscriptcsgo")) {
                   //}
        }
      
        /* nk_label(ctx, "2020 All Rights Reserved", NK_TEXT_LEFT);*/
       /*   nk_layout_row_static(ctx, 20, 50, 5);
        if (nk_button_label(ctx, "github")) {
            ShellExecute(NULL, "open", "https://vk.com/bunnyscriptcsgo", NULL, NULL, SW_SHOWNORMAL);
        }*/
        nk_end(ctx);

#ifdef INCLUDE_CALCULATOR
        calculator(ctx);
#endif
#ifdef INCLUDE_OVERVIEW
        overview(ctx);
#endif
#ifdef INCLUDE_NODE_EDITOR
        node_editor(ctx);
#endif

        {
            HRESULT hr;
            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                D3DCOLOR_COLORVALUE(bg.r, bg.g, bg.b, bg.a), 0.0f, 0);
            NK_ASSERT(SUCCEEDED(hr));

            hr = IDirect3DDevice9_BeginScene(device);
            NK_ASSERT(SUCCEEDED(hr));
            nk_d3d9_render(NK_ANTI_ALIASING_ON);
            hr = IDirect3DDevice9_EndScene(device);
            NK_ASSERT(SUCCEEDED(hr));

            if (deviceEx) {
                hr = IDirect3DDevice9Ex_PresentEx(deviceEx, NULL, NULL, NULL, NULL, 0);
            }
            else {
                hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            }
            if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED) {
                MessageBoxW(NULL, L"D3D9 device is lost or removed!", L"Error", 0);
                break;
            }
            else if (hr == S_PRESENT_OCCLUDED) {
                Sleep(10);
            }
            NK_ASSERT(SUCCEEDED(hr));
        }
    }
    nk_d3d9_shutdown();
    if (deviceEx)IDirect3DDevice9Ex_Release(deviceEx);
    else IDirect3DDevice9_Release(device);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
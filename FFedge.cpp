// FFedge
// License: MIT

// Detect memory leaks (MSVC Debug only)
#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include <strsafe.h>
#ifdef _MSC_VER
    #include "WebView2.h"
#else
    #include "webview2/WebView2.h"
#endif

#define CLASSNAME _T("FFedge by katahiromz")
#define TITLE _T("FFedge")
#define URL L"https://google.com/"

HINSTANCE g_hInst = nullptr;
static ICoreWebView2Controller *g_webviewController = nullptr;
static ICoreWebView2 *g_webView = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////
// message

HWND g_hMainWnd = NULL;

BOOL message_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LPCWSTR psz = (LPCWSTR)lParam;
    SetDlgItemTextW(hwnd, edt1, psz);
    return TRUE;
}

void message_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    }
}

INT_PTR CALLBACK
message_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, message_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, message_OnCommand);
    }
    return 0;
}

void message(INT uType, LPCWSTR fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    WCHAR buf[1024];
    StringCchVPrintfW(buf, _countof(buf), fmt, va);
    DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), NULL, message_DialogProc, (LPARAM)buf);
    va_end(va);
}

void version(void)
{
    message(MB_ICONINFORMATION, L"FFedge version 1.1 by katahiromz");
}

void usage(void)
{
    message(MB_ICONINFORMATION,
        L"Usage: FFedge [Options] URL\r\n"
        L"\r\n"
        L"Options:\r\n"
        L"  -i URL                Specify an internet location or an HTML file.\r\n"
        L"  -x WIDTH              Set the displayed width.\r\n"
        L"  -y HEIGHT             Set the displayed height.\r\n"
        L"  -left LEFT            Specify the x position of the window's left edge\r\n"
        L"                        (default is centered).\r\n"
        L"  -top TOP              Specify the y position of the window's top edge\r\n"
        L"                        (default is centered).\r\n"
        L"  -fs                   Start in fullscreen mode.\r\n"
        L"  -noborder             Create a borderless window.\r\n"
        L"  -window_title TITLE   Set the window title (default is FFedge)\r\n"
        L"  -help                 Display this help message.\r\n"
        L"  -version              Display version information.\r\n"
    );
}

///////////////////////////////////////////////////////////////////////////////////////////////

class MCoreWebView2HandlersImpl
    : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    , public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    , public ICoreWebView2WebMessageReceivedEventHandler
{
protected:
    LONG m_cRefs = 1; // reference counter
    HWND m_hWnd = nullptr;
    std::wstring m_url;

public:
    MCoreWebView2HandlersImpl(HWND hWnd, LPCWSTR url) : m_hWnd(hWnd), m_url(url) {  }

    // ICoreWebView2CreateCoreWebView2ControllerCompletedHandler interface
    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (FAILED(result) || !controller)
            return result;

        g_webviewController = controller;
        g_webviewController->AddRef();

        g_webviewController->get_CoreWebView2(&g_webView);
        if (!g_webView) return E_FAIL;

        // Configure settings
        ICoreWebView2Settings* settings = nullptr;
        g_webView->get_Settings(&settings);
        if (settings) {
            settings->put_AreDefaultContextMenusEnabled(TRUE);
            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            settings->put_IsBuiltInErrorPageEnabled(TRUE);
            settings->put_IsScriptEnabled(TRUE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_IsWebMessageEnabled(TRUE);
            settings->put_IsZoomControlEnabled(FALSE);
            settings->Release();
        }

        // Resize WebView
        PostMessage(m_hWnd, WM_SIZE, 0, 0);

        // Navigate to URL
        g_webView->Navigate(m_url.c_str());

        return S_OK;
    }

    // ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler interface
    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Environment* env) override {
        if (FAILED(result) || !env) 
            return result;

        env->CreateCoreWebView2Controller(m_hWnd, this);
        return S_OK;
    }

    // ICoreWebView2WebMessageReceivedEventHandler interface
    STDMETHODIMP Invoke(
        ICoreWebView2* sender,
        ICoreWebView2WebMessageReceivedEventArgs* args) override
    {
        // Get the message as a string
        LPWSTR message = nullptr;
        HRESULT hr = args->TryGetWebMessageAsString(&message);
        if (SUCCEEDED(hr) && message) {
            std::wstring msg(message);
            CoTaskMemFree(message);

            msg += L"\n";
            OutputDebugStringW(msg.c_str()); // Debug output
        }

        return S_OK;
    }

    // IUnknown interface
    STDMETHODIMP_(ULONG) AddRef() override { return ++m_cRefs; }
    STDMETHODIMP_(ULONG) Release() override {
        if (--m_cRefs == 0) {
            delete this;
            return 0;
        }
        return m_cRefs;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown ||
            riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ||
            riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler ||
            riid == IID_ICoreWebView2WebMessageReceivedEventHandler)
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////

struct FFEDGE
{
    bool m_noborder = false, m_fullscreen = false;
    std::wstring m_url = L"https://google.com";
    std::wstring m_window_title = L"FFedge";
    INT m_x = CW_USEDEFAULT, m_y = CW_USEDEFAULT;
    INT m_left = CW_USEDEFAULT, m_top = CW_USEDEFAULT;
    HWND m_hWnd;

    INT parse_command_line(INT argc, LPWSTR* argv);
    INT run(HINSTANCE hInstance, INT nCmdShow);

protected:
    BOOL registerClasses(void);
    BOOL createMainWnd(INT nCmdShow);
};

#define MY_WM_CREATE_WEBVIEW (WM_USER + 100)

// The window procedure
LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        {
            FFEDGE *pThis = (FFEDGE *)(((CREATESTRUCT*)lParam)->lpCreateParams);
            pThis->m_hWnd = hWnd;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            PostMessage(hWnd, MY_WM_CREATE_WEBVIEW, 0, 0);
        }
        return 0;
    case MY_WM_CREATE_WEBVIEW:
        {
            FFEDGE *pThis = (FFEDGE *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            auto handler = new MCoreWebView2HandlersImpl(hWnd, pThis->m_url.c_str());
            CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, handler);
            handler->Release();
        }
        return 0;
    case WM_DESTROY:
        if (g_webviewController) {
            g_webviewController->Release();
            g_webviewController = nullptr;
        }
        if (g_webView) {
            g_webView->Release();
            g_webView = nullptr;
        }
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (g_webviewController) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            g_webviewController->put_Bounds(bounds);
        }
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT FFEDGE::parse_command_line(INT argc, LPWSTR *argv)
{
    for (INT iarg = 1; iarg < argc; ++iarg)
    {
        LPWSTR arg = argv[iarg];

        if (lstrcmpiW(arg, L"-help") == 0 || lstrcmpiW(arg, L"--help") == 0)
        {
            usage();
            return 1;
        }

        if (lstrcmpiW(arg, L"-version") == 0 || lstrcmpiW(arg, L"--version") == 0)
        {
            version();
            return 1;
        }

        if (lstrcmpiW(arg, L"-x") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_x = (INT)wcstoul(arg, NULL, 0);
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -x needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-y") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_y = (INT)wcstoul(arg, NULL, 0);
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -y needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-left") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_left = (INT)wcstol(arg, NULL, 0);
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -left needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-top") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_top = (INT)wcstol(arg, NULL, 0);
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -top needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-window_title") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_window_title = arg;
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -window_title needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-i") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_url = arg;
                continue;
            }
            else
            {
                message(MB_ICONERROR, L"ERROR: Option -i needs an option.");
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-fs") == 0)
        {
            m_fullscreen = true;
            continue;
        }

        if (lstrcmpiW(arg, L"-noborder") == 0)
        {
            m_noborder = true;
            continue;
        }

        if (arg[0] == L'-')
        {
            message(MB_ICONERROR, L"ERROR: '%s' is invalid argument.", arg);
            return 1;
        }

        m_url = arg;
    }

    return 0;
}

BOOL FFEDGE::registerClasses(void)
{
    WNDCLASSEXW wcx = { sizeof(wcx) };
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = WindowProc;
    wcx.hInstance = GetModuleHandleW(NULL);
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = NULL;
    wcx.lpszClassName = CLASSNAME;
    wcx.hIconSm = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        0);
    return RegisterClassExW(&wcx);
}

BOOL FFEDGE::createMainWnd(INT nCmdShow)
{
    DWORD style, exstyle = WS_EX_TOPMOST;

    if (m_noborder || m_fullscreen)
        style = WS_POPUPWINDOW;
    else
        style = WS_OVERLAPPEDWINDOW;

    RECT rcWork = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    SIZE sizWork = { (rcWork.right - rcWork.left), (rcWork.bottom - rcWork.top) };

    if (m_x <= 0)
        m_x = CW_USEDEFAULT;
    if (m_y <= 0)
        m_y = CW_USEDEFAULT;

    if (m_fullscreen)
    {
        m_left = rcWork.left;
        m_top = rcWork.top;
        m_x = sizWork.cx;
        m_y = sizWork.cy;
    }
    else
    {
        if (m_x != CW_USEDEFAULT && m_y == CW_USEDEFAULT)
        {
            m_y = m_x * 600 / 800;
        }
        else if (m_x == CW_USEDEFAULT && m_y != CW_USEDEFAULT)
        {
            m_x = m_y * 800 / 600;
        }
        else if (m_x == CW_USEDEFAULT && m_y == CW_USEDEFAULT)
        {
            m_x = sizWork.cx * 2 / 3;
            m_y = sizWork.cy * 2 / 3;
        }

        if (m_left == CW_USEDEFAULT)
            m_left = (sizWork.cx - m_x) / 2;
        if (m_top == CW_USEDEFAULT)
            m_top  = (sizWork.cy - m_y) / 2;
    }

    RECT rc;
    rc.left = m_left;
    rc.top = m_top;
    rc.right = m_left + m_x;
    rc.bottom = m_top + m_y;
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);

    CreateWindowExW(exstyle, CLASSNAME, m_window_title.c_str(), style,
        rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, GetModuleHandleW(NULL), this);
    if (!m_hWnd)
        return FALSE;

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return TRUE;
}

INT FFEDGE::run(HINSTANCE hInstance, INT nCmdShow)
{
    g_hInst = hInstance;

    if (m_url.empty())
    {
        message(MB_ICONERROR, L"ERROR: No URL specified.");
        return -1;
    }

    if (m_window_title.empty())
        m_window_title = L"FFedge";

    if (!registerClasses())
    {
        message(MB_ICONERROR, L"ERROR: Failed to register classes.");
        return -2;
    }

    if (!createMainWnd(nCmdShow))
    {
        message(MB_ICONERROR, L"ERROR: Failed to create the main window.");
        return -3;
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (INT)msg.wParam;
}

INT FFedge_main(
    HINSTANCE hInstance,
    INT argc,
    LPWSTR *argv,
    INT nCmdShow)
{
    InitCommonControls();

    FFEDGE ffedge;
    INT ret = ffedge.parse_command_line(argc, argv);
    if (ret)
        return ret;

    return ffedge.run(hInstance, nCmdShow);
}

// The Windows app main function
INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    INT       nCmdShow)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = FFedge_main(hInstance, argc, argv, nCmdShow);
    LocalFree(argv);

#if (_WIN32_WINNT >= 0x0500) && !defined(NDEBUG)
    // Detect handle leaks (Debug only)
    TCHAR szText[MAX_PATH];
    wnsprintf(szText, _countof(szText), TEXT("GDI Objects: %ld, User Objects: %ld\n"),
              GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS),
              GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS));
    OutputDebugString(szText);
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // Detect memory leaks (MSVC Debug only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return ret;
}

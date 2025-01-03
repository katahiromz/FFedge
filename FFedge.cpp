// FFedge
// License: MIT

#define _WIN32_WINNT _WIN32_WINNT_VISTA

// Detect memory leaks (MSVC Debug only)
#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
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
HWND g_hMainWnd = nullptr;
static ICoreWebView2Controller *g_webviewController = nullptr;
static ICoreWebView2 *g_webView = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////

inline WORD get_lang_id(void)
{
    return PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
}

// localization
LPCTSTR get_text(INT id)
{
#ifdef JAPAN
    if (get_lang_id() == LANG_JAPANESE) // Japone for Japone
    {
        switch (id)
        {
        case 0: return TEXT("FFedge バージョン 1.2 by 片山博文MZ");
        case 1:
            return
                TEXT("使用方法: FFedge [オプション] URL\n")
                TEXT("\n")
                TEXT("オプション:\n")
                TEXT(" -i URL                  インターネット上の位置またはHTMLファイルを指定します。\n")
                TEXT(" -x 幅                   表示される幅を設定します。\n")
                TEXT(" -y 高さ                 表示される高さを設定します。\n")
                TEXT(" -left 左                ウィンドウの左端のx位置を指定します(デフォルト:中央)。\n")
                TEXT(" -top 上                 ウィンドウの上端のy位置を指定します(デフォルト:中央)。\n")
                TEXT(" -fs                     フルスクリーンモードで開始します。\n")
                TEXT(" -noborder               枠なしウィンドウを作成します。\n")
                TEXT(" -window_title タイトル  ウィンドウタイトルを設定します(デフォルト:FFedge)。\n")
                TEXT(" -auto-click             自動的にクリックします(音楽再生用)。\n")
                TEXT(" -help                   このヘルプメッセージを表示します。\n")
                TEXT(" -version                バージョン情報を表示します。\n");
        case 2: return TEXT("エラー: オプション -x は引数が必要です\n");
        case 3: return TEXT("エラー: オプション -y は引数が必要です\n");
        case 4: return TEXT("エラー: オプション -left は引数が必要です\n");
        case 5: return TEXT("エラー: オプション -top  は引数が必要です\n");
        case 6: return TEXT("エラー: オプション -window_title は引数が必要です\n");
        case 7: return TEXT("エラー: オプション -i は引数が必要です\n");
        case 8: return TEXT("エラー: '%s' は無効な引数です\n");
        case 9: return TEXT("エラー: クラス登録に失敗しました\n");
        case 10: return TEXT("エラー: メインウィンドウ作成に失敗しました\n");
        case 11: return TEXT("エラー: URLが未指定です\n");
        }
    }
    else // The others are Let's la English
#endif
    {
        switch (id)
        {
        case 0: return TEXT("FFedge version 1.2 by katahiromz");
        case 1:
            return
                TEXT("Usage: FFedge [Options] URL\n")
                TEXT("\n")
                TEXT("Options:\n")
                TEXT("  -i URL                Specify an internet location or an HTML file.\n")
                TEXT("  -x WIDTH              Set the displayed width.\n")
                TEXT("  -y HEIGHT             Set the displayed height.\n")
                TEXT("  -left LEFT            Specify the x position of the window's left edge\n")
                TEXT("                        (default is centered).\n")
                TEXT("  -top TOP              Specify the y position of the window's top edge\n")
                TEXT("                        (default is centered).\n")
                TEXT("  -fs                   Start in fullscreen mode.\n")
                TEXT("  -noborder             Create a borderless window.\n")
                TEXT("  -window_title TITLE   Set the window title (default is FFedge)\n")
                TEXT("  -auto_click           Auto click (for music play).\n")
                TEXT("  -help                 Display this help message.\n")
                TEXT("  -version              Display version information.\n");
        case 2: return TEXT("ERROR: Option -x needs an operand.\n");
        case 3: return TEXT("ERROR: Option -y needs an operand.\n");
        case 4: return TEXT("ERROR: Option -left needs an operand.\n");
        case 5: return TEXT("ERROR: Option -top needs an operand.\n");
        case 6: return TEXT("ERROR: Option -window_title needs an operand.\n");
        case 7: return TEXT("ERROR: Option -i needs an operand.\n");
        case 8: return TEXT("ERROR: '%s' is invalid argument.\n");
        case 9: return TEXT("ERROR: Failed to register classes.\n");
        case 10: return TEXT("ERROR: Failed to create the main window.\n");
        case 11: return TEXT("ERROR: No URL specified.\n");
        }
    }

    assert(0);
    return nullptr;
}

void version(void)
{
    _putts(get_text(0));
}

void usage(void)
{
    _putts(get_text(1));
}

///////////////////////////////////////////////////////////////////////////////////////////////

class MCoreWebView2HandlersImpl
    : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    , public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    , public ICoreWebView2WebMessageReceivedEventHandler
    , public ICoreWebView2NavigationCompletedEventHandler
{
protected:
    LONG m_cRefs = 1; // reference counter
    HWND m_hWnd = nullptr;
    std::wstring m_url;
    BOOL m_auto_click = false;

public:
    MCoreWebView2HandlersImpl(HWND hWnd, LPCWSTR url, BOOL auto_click = FALSE)
        : m_hWnd(hWnd)
        , m_url(url)
        , m_auto_click(auto_click) {  }

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

        // Receive NavigationCompleted event
        EventRegistrationToken token;
        g_webView->add_NavigationCompleted(this, &token);

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

    // SendInput用に変換
    POINT convert_pt_for_SendInput(POINT pt) const {
        return {
            (pt.x * 65536 + GetSystemMetrics(SM_CXSCREEN) - 1) / GetSystemMetrics(SM_CXSCREEN),
            (pt.y * 65536 + GetSystemMetrics(SM_CYSCREEN) - 1) / GetSystemMetrics(SM_CYSCREEN)
        };
    }

    // ICoreWebView2NavigationCompletedEventHandler interface
    STDMETHODIMP Invoke( 
        ICoreWebView2 *sender,
        ICoreWebView2NavigationCompletedEventArgs *args) override
    {
        if (m_auto_click) {
            // カーソルの位置を覚えておく
            POINT old_pt;
            GetCursorPos(&old_pt);

            // 中央位置を取得
            RECT rc;
            GetWindowRect(m_hWnd, &rc);
            POINT center = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };
            POINT converted = convert_pt_for_SendInput(center);

            // マウス入力をエミュレート
            // https://qiita.com/kob58im/items/23df9e22778b33986d1c
            INPUT input;
            ZeroMemory(&input, sizeof(input));
            input.type = INPUT_MOUSE;
            input.mi.dx = converted.x;
            input.mi.dy = converted.y;
            input.mi.mouseData = 0;
            input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            SendInput(1, &input, sizeof(input));
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.mouseData = 0;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(input));
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.mouseData = 0;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(input));

            // 元に戻す
            SetCursorPos(old_pt.x, old_pt.y);
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
            riid == IID_ICoreWebView2WebMessageReceivedEventHandler ||
            riid == IID_ICoreWebView2NavigationCompletedEventHandler)
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
    HWND m_hWnd = nullptr;
    bool m_auto_click = false;

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
            auto handler = new MCoreWebView2HandlersImpl(hWnd, pThis->m_url.c_str(), pThis->m_auto_click);
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

        if (lstrcmpiW(arg, L"-x") == 0 || lstrcmpiW(arg, L"--x") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_x = (INT)wcstoul(arg, nullptr, 0);
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(2));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-y") == 0 || lstrcmpiW(arg, L"--y") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_y = (INT)wcstoul(arg, nullptr, 0);
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(3));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-left") == 0 || lstrcmpiW(arg, L"--left") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_left = (INT)wcstol(arg, nullptr, 0);
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(4));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-top") == 0 || lstrcmpiW(arg, L"--top") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_top = (INT)wcstol(arg, nullptr, 0);
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(5));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-window_title") == 0 || lstrcmpiW(arg, L"--window_title") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_window_title = arg;
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(6));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-i") == 0 || lstrcmpiW(arg, L"--i") == 0)
        {
            arg = argv[++iarg];
            if (iarg < argc)
            {
                m_url = arg;
                continue;
            }
            else
            {
                _ftprintf(stderr, get_text(7));
                return 1;
            }
        }

        if (lstrcmpiW(arg, L"-fs") == 0 || lstrcmpiW(arg, L"--fs") == 0)
        {
            m_fullscreen = true;
            continue;
        }

        if (lstrcmpiW(arg, L"-noborder") == 0 || lstrcmpiW(arg, L"--noborder") == 0)
        {
            m_noborder = true;
            continue;
        }

        if (lstrcmpiW(arg, L"-auto_click") == 0 || lstrcmpiW(arg, L"--auto_click") == 0)
        {
            m_auto_click = true;
            continue;
        }

        if (arg[0] == L'-')
        {
            _ftprintf(stderr, get_text(8), arg);
            return 1;
        }

        if (PathFileExists(arg))
        {
            TCHAR full_name[MAX_PATH];
            GetFullPathName(arg, _countof(full_name), full_name, nullptr);
            m_url = TEXT("file:");
            m_url += full_name;
            continue;
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
    wcx.hInstance = GetModuleHandleW(nullptr);
    wcx.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcx.hbrBackground = nullptr;
    wcx.lpszClassName = CLASSNAME;
    wcx.hIconSm = (HICON)LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON,
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
        nullptr, nullptr, GetModuleHandleW(nullptr), this);
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
        _ftprintf(stderr, get_text(11));
        return -1;
    }

    if (m_window_title.empty())
        m_window_title = L"FFedge";

    if (!registerClasses())
    {
        _ftprintf(stderr, get_text(9));
        return -2;
    }

    if (!createMainWnd(nCmdShow))
    {
        _ftprintf(stderr, get_text(10));
        return -3;
    }

    FreeConsole();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
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

#include <clocale>

int main(void)
{
    // Unicode console output support
    std::setlocale(LC_ALL, "");

    // 高解像度対応 (Vista+)
    // https://qiita.com/kob58im/items/23df9e22778b33986d1c
    SetProcessDPIAware();

    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = FFedge_main(GetModuleHandle(nullptr), argc, argv, SW_SHOWNORMAL);
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

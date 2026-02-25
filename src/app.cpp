#include "app.h"
#include "i18n.h"
#include "logger.h"
#include "utils.h"
#include "tab_view.h"
#include <windows.h>
#include <dwmapi.h>
#include <objbase.h>
#include <commctrl.h>

#pragma comment(lib, "dwmapi.lib")

namespace fto {

bool is_dark_mode() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    DWORD value = 1;
    DWORD size = sizeof(value);
    RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);
    return value == 0; // 0 = dark mode
}

void apply_dark_title_bar(HWND hwnd) {
    BOOL dark = is_dark_mode() ? TRUE : FALSE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
}

App::App() {}
App::~App() {}

bool App::init() {
    // Initialize COM (STA for UI Automation)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"COM initialization failed.", L"Error", MB_ICONERROR);
        return false;
    }

    // Initialize Common Controls v6
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icc);

    // Initialize logger
    std::wstring log_dir = get_appdata_dir();
    Logger::instance().init(log_dir);
    LOG_INFO("app", "=== FileTabOpenerW starting ===");

    // Initialize i18n
    i18n_init();

    // Load config
    config_.load();

    // Apply saved language if present
    auto saved_lang = config_.get_setting("language");
    if (!saved_lang.empty()) {
        std::wstring wlang = utf8_to_wide(saved_lang);
        set_language(wlang);
        LOG_INFO("app", "Language from config: %s", saved_lang.c_str());
    }
    LOG_INFO("app", "Active language: %s", wide_to_utf8(get_language()).c_str());

    // Register custom window classes
    MainWindow::register_class();
    TabView::register_class();

    // Create main window
    window_ = std::make_unique<MainWindow>(config_);
    window_->create();

    LOG_INFO("app", "Initialization complete");
    return true;
}

int App::run() {
    if (!window_) return 1;
    window_->run();

    LOG_INFO("app", "=== FileTabOpenerW exiting ===");
    CoUninitialize();
    return 0;
}

} // namespace fto

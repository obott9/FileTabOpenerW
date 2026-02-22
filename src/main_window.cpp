#include "main_window.h"
#include "explorer_opener.h"
#include "i18n.h"
#include "utils.h"
#include "logger.h"
#include "res/resource.h"
#include <thread>
#include <sstream>
#include <commctrl.h>
#include <objbase.h>

namespace fto {

static const wchar_t* MAIN_WINDOW_CLASS = L"FileTabOpenerMainWindow";

void MainWindow::register_class() {
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = MAIN_WINDOW_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);
}

MainWindow::MainWindow(ConfigManager& config)
    : config_(config) {}

void MainWindow::create() {
    // Parse saved geometry
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 800, h = 600;
    auto geom = config_.data().window_geometry;
    if (!geom.empty()) {
        // Format: "WxH+X+Y" or "WxH"
        int gw = 0, gh = 0, gx = 0, gy = 0;
        if (sscanf(geom.c_str(), "%dx%d+%d+%d", &gw, &gh, &gx, &gy) >= 2) {
            w = gw; h = gh;
            if (gx || gy) { x = gx; y = gy; }
        }
    }

    hwnd_ = CreateWindowExW(0, MAIN_WINDOW_CLASS,
        t("app.title").c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, w, h,
        nullptr, nullptr, GetModuleHandleW(nullptr), this);

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
}

void MainWindow::run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        // Handle Enter key in path entry
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND focus = GetFocus();
            LONG id = GetWindowLongW(focus, GWL_ID);
            if (id == IDC_PATH_ENTRY && tab_group_) {
                tab_group_->handle_command(MAKEWPARAM(IDC_PATH_ADD_BTN, BN_CLICKED), 0);
                continue;
            }
        }
        if (!IsDialogMessageW(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_CREATE:
        self->on_create();
        return 0;
    case WM_SIZE:
        self->on_size(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = 600;
        mmi->ptMinTrackSize.y = 400;
        return 0;
    }
    case WM_COMMAND:
        self->on_command(wParam, lParam);
        return 0;
    case WM_TAB_OPEN_COMPLETE:
        self->on_tab_open_complete();
        return 0;
    case WM_TAB_OPEN_ERROR: {
        // wParam = path string pointer, lParam = error string pointer
        auto* path = reinterpret_cast<std::wstring*>(wParam);
        auto* error = reinterpret_cast<std::wstring*>(lParam);
        if (path && error) self->on_tab_open_error(*path, *error);
        delete path;
        delete error;
        return 0;
    }
    case WM_CLOSE:
        self->on_close();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void MainWindow::on_create() {
    font_ = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // --- Settings bar (right-aligned) ---
    // These are created with approximate positions; on_size will fix them.
    timeout_label_ = CreateWindowW(L"STATIC", t("settings.timeout").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_RIGHT,
        0, 0, 0, 0, hwnd_, (HMENU)(INT_PTR)IDC_SETTINGS_TIMEOUT_LABEL, nullptr, nullptr);
    SendMessageW(timeout_label_, WM_SETFONT, (WPARAM)font_, TRUE);

    timeout_combo_ = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 120, hwnd_, (HMENU)(INT_PTR)IDC_SETTINGS_TIMEOUT_COMBO, nullptr, nullptr);
    SendMessageW(timeout_combo_, WM_SETFONT, (WPARAM)font_, TRUE);
    for (auto* val : {L"5", L"10", L"15", L"30", L"60"}) {
        SendMessageW(timeout_combo_, CB_ADDSTRING, 0, (LPARAM)val);
    }
    int saved_timeout = config_.get_timeout();
    std::wstring to_str = std::to_wstring(saved_timeout);
    int idx = (int)SendMessageW(timeout_combo_, CB_FINDSTRINGEXACT, -1, (LPARAM)to_str.c_str());
    if (idx >= 0) SendMessageW(timeout_combo_, CB_SETCURSEL, idx, 0);
    else SendMessageW(timeout_combo_, CB_SETCURSEL, 3, 0); // default 30

    timeout_unit_ = CreateWindowW(L"STATIC", t("settings.timeout_unit").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd_, (HMENU)(INT_PTR)IDC_SETTINGS_TIMEOUT_UNIT, nullptr, nullptr);
    SendMessageW(timeout_unit_, WM_SETFONT, (WPARAM)font_, TRUE);

    lang_label_ = CreateWindowW(L"STATIC", L"\xD83C\xDF10",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd_, (HMENU)(INT_PTR)IDC_SETTINGS_LANG_LABEL, nullptr, nullptr);
    SendMessageW(lang_label_, WM_SETFONT, (WPARAM)font_, TRUE);

    lang_combo_ = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 150, hwnd_, (HMENU)(INT_PTR)IDC_SETTINGS_LANG_COMBO, nullptr, nullptr);
    SendMessageW(lang_combo_, WM_SETFONT, (WPARAM)font_, TRUE);
    int lang_idx = 0;
    for (int i = 0; i < (int)supported_langs().size(); ++i) {
        const auto& li = supported_langs()[i];
        SendMessageW(lang_combo_, CB_ADDSTRING, 0, (LPARAM)li.display_name);
        if (get_language() == li.code) lang_idx = i;
    }
    SendMessageW(lang_combo_, CB_SETCURSEL, lang_idx, 0);

    // Separator
    separator_ = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        0, 0, 0, 2, hwnd_, nullptr, nullptr, nullptr);

    // History section
    history_ = std::make_unique<HistorySection>(config_,
        [this](const std::wstring& path) { open_single_folder(path); });

    // Tab group section
    tab_group_ = std::make_unique<TabGroupSection>(config_,
        [this](const std::vector<std::wstring>& paths, const std::optional<WindowRect>& rect) {
            open_folders_as_tabs(paths, rect);
        });

    // Initial layout will happen in WM_SIZE
}

void MainWindow::on_size(int w, int h) {
    client_w_ = w;
    client_h_ = h;
    int pad = 10, row_h = 28;
    int cy = 5;

    // Settings bar (right-aligned)
    int right = w - pad;
    int combo_h = 22;

    // Language combo (rightmost)
    int lang_w = 100;
    MoveWindow(lang_combo_, right - lang_w, cy, lang_w, combo_h + 100, TRUE);
    right -= lang_w + 3;
    MoveWindow(lang_label_, right - 20, cy, 20, row_h, TRUE);
    right -= 20 + 10;

    // Timeout
    int unit_w = 30;
    MoveWindow(timeout_unit_, right - unit_w, cy, unit_w, row_h, TRUE);
    right -= unit_w + 1;
    int to_combo_w = 50;
    MoveWindow(timeout_combo_, right - to_combo_w, cy, to_combo_w, combo_h + 100, TRUE);
    right -= to_combo_w + 3;
    int to_label_w = 70;
    MoveWindow(timeout_label_, right - to_label_w, cy, to_label_w, row_h, TRUE);

    cy += row_h + 5;

    // History section
    if (history_) {
        if (!history_->is_created()) {  // Not yet created
            history_->create(hwnd_, pad, cy, w - pad * 2, row_h);
        } else {
            history_->resize(pad, cy, w - pad * 2, row_h);
        }
    }
    cy += row_h + 5;

    // Separator
    MoveWindow(separator_, pad, cy, w - pad * 2, 2, TRUE);
    cy += 7;

    // Tab group section (fills the rest)
    int remaining = h - cy - 5;
    if (tab_group_) {
        if (!IsWindow(tab_group_->tab_view().hwnd())) {
            tab_group_->create(hwnd_, pad, cy, w - pad * 2, remaining);
        } else {
            tab_group_->resize(pad, cy, w - pad * 2, remaining);
        }
    }

    // Toast overlay
    if (toast_hwnd_ && IsWindow(toast_hwnd_)) {
        int tw = w / 2, th = h / 2;
        MoveWindow(toast_hwnd_, (w - tw) / 2, (h - th) / 2, tw, th, TRUE);
    }
}

void MainWindow::on_close() {
    // Save geometry
    RECT r;
    GetWindowRect(hwnd_, &r);
    int gw = r.right - r.left, gh = r.bottom - r.top;
    char buf[64];
    snprintf(buf, sizeof(buf), "%dx%d+%d+%d", gw, gh, r.left, r.top);
    config_.data().window_geometry = buf;

    // Save tab geometry
    if (tab_group_) {
        // Trigger geometry save for current tab
        tab_group_->handle_command(MAKEWPARAM(IDC_GEOM_GET_BTN + 1, 0), 0); // no-op, just save via on_close
    }

    config_.save();
    LOG_INFO("main_window", "Window closed (geometry: %s)", buf);
    DestroyWindow(hwnd_);
}

void MainWindow::on_command(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);

    // Timeout changed
    if (id == IDC_SETTINGS_TIMEOUT_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
        wchar_t buf[16];
        int sel = (int)SendMessageW(timeout_combo_, CB_GETCURSEL, 0, 0);
        SendMessageW(timeout_combo_, CB_GETLBTEXT, sel, (LPARAM)buf);
        config_.set_setting("timeout", wide_to_utf8(buf));
        config_.save();
        LOG_INFO("main_window", "Timeout changed to %s", wide_to_utf8(buf).c_str());
        return;
    }

    // Language changed
    if (id == IDC_SETTINGS_LANG_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
        int sel = (int)SendMessageW(lang_combo_, CB_GETCURSEL, 0, 0);
        if (sel >= 0 && sel < (int)supported_langs().size()) {
            auto code = supported_langs()[sel].code;
            if (code != get_language()) {
                set_language(code);
                config_.set_setting("language", wide_to_utf8(code));
                config_.save();
                SetWindowTextW(hwnd_, t("app.title").c_str());
                // Full rebuild would require recreating all controls.
                // For simplicity, just update the title. Full i18n refresh
                // would require destroying and recreating all child windows.
                LOG_INFO("main_window", "Language changed to %s (restart for full effect)",
                         wide_to_utf8(code).c_str());
            }
        }
        return;
    }

    // Forward to sections
    if (history_ && history_->handle_command(wParam, lParam)) return;
    if (tab_group_ && tab_group_->handle_command(wParam, lParam)) return;
}

void MainWindow::open_single_folder(const std::wstring& path) {
    LOG_INFO("main_window", "Opening single folder: %s", wide_to_utf8(path).c_str());
    fto::open_single_folder(path);
}

void MainWindow::open_folders_as_tabs(
    const std::vector<std::wstring>& paths,
    const std::optional<WindowRect>& rect)
{
    LOG_INFO("main_window", "Opening %d folders as tabs", (int)paths.size());

    auto [valid, invalid] = fto::validate_paths(paths);
    if (!invalid.empty()) {
        std::wstring msg;
        for (const auto& p : invalid) msg += p + L"\n";
        MessageBoxW(hwnd_, t("error.invalid_paths_msg", {{"paths", msg}}).c_str(),
                     t("error.invalid_paths_title").c_str(), MB_ICONWARNING);
    }
    if (valid.empty()) return;

    float timeout = (float)config_.get_timeout();
    SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    show_toast();

    HWND main_hwnd = hwnd_;
    std::thread([valid, rect, timeout, main_hwnd]() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        try {
            fto::open_folders_as_tabs(valid,
                nullptr, // on_progress
                [main_hwnd](const std::wstring& p, const std::wstring& e) {
                    PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                        (WPARAM)new std::wstring(p),
                        (LPARAM)new std::wstring(e));
                },
                timeout, rect);
        } catch (const std::exception& e) {
            std::wstring err = utf8_to_wide(e.what());
            PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                (WPARAM)new std::wstring(L""),
                (LPARAM)new std::wstring(err));
        }
        CoUninitialize();
        PostMessageW(main_hwnd, WM_TAB_OPEN_COMPLETE, 0, 0);
    }).detach();
}

void MainWindow::on_tab_open_complete() {
    hide_toast();
    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
    if (tab_group_) tab_group_->set_opening(false);
    LOG_INFO("main_window", "Tab opening complete");
}

void MainWindow::on_tab_open_error(const std::wstring& path, const std::wstring& error) {
    MessageBoxW(hwnd_,
        t("error.open_failed", {{"path", path}, {"error", error}}).c_str(),
        t("error.title").c_str(), MB_ICONERROR);
}

void MainWindow::show_toast() {
    hide_toast();
    toast_hwnd_ = CreateWindowExW(0, L"STATIC",
        t("toast.opening_tabs").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd_, nullptr, nullptr, nullptr);
    // Position in center
    int tw = client_w_ / 2, th = client_h_ / 2;
    MoveWindow(toast_hwnd_, (client_w_ - tw) / 2, (client_h_ - th) / 2, tw, th, TRUE);
    SendMessageW(toast_hwnd_, WM_SETFONT, (WPARAM)font_, TRUE);
    // Raise to top
    SetWindowPos(toast_hwnd_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void MainWindow::hide_toast() {
    if (toast_hwnd_ && IsWindow(toast_hwnd_)) {
        DestroyWindow(toast_hwnd_);
        toast_hwnd_ = nullptr;
    }
}

} // namespace fto

#include "main_window.h"
#include "app.h"
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
    // Load custom icon if available, otherwise use default
    wc.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
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
        if (sscanf_s(geom.c_str(), "%dx%d+%d+%d", &gw, &gh, &gx, &gy) >= 2) {
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
    case WM_DRAWITEM: {
        auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (dis->CtlType == ODT_BUTTON) {
            COLORREF bg = self->dark_mode_ ? theme::BTN_BG_DARK : theme::BTN_BG_LIGHT;
            COLORREF pressed = self->dark_mode_ ? theme::BTN_PRESSED_DARK : theme::BTN_PRESSED_LIGHT;
            COLORREF text = self->dark_mode_ ? theme::BTN_TEXT_DARK : theme::BTN_TEXT_LIGHT;
            COLORREF parent_bg = self->dark_mode_ ? theme::PARENT_BG_DARK : GetSysColor(COLOR_3DFACE);
            draw_themed_button(dis, bg, pressed, text, parent_bg);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HBRUSH br = self->handle_ctlcolor((HDC)wParam, (HWND)lParam);
        if (br) return (LRESULT)br;
        break;
    }
    case WM_SETTINGCHANGE:
        // Detect Windows theme change
        if (lParam && wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) {
            self->update_dark_mode();
        }
        break;
    case WM_CLOSE:
        self->on_close();
        return 0;
    case WM_DESTROY:
        if (self->font_) DeleteObject(self->font_);
        if (self->dark_bg_brush_) DeleteObject(self->dark_bg_brush_);
        if (self->dark_edit_brush_) DeleteObject(self->dark_edit_brush_);
        if (self->toast_font_) DeleteObject(self->toast_font_);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void MainWindow::on_create() {
    font_ = create_system_message_font();

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
    int idx = (int)SendMessageW(timeout_combo_, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)to_str.c_str());
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

    // Initialize dark mode
    update_dark_mode();

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
    closing_ = true;

    // Save window geometry
    RECT r;
    GetWindowRect(hwnd_, &r);
    int gw = r.right - r.left, gh = r.bottom - r.top;
    char buf[64];
    snprintf(buf, sizeof(buf), "%dx%d+%d+%d", gw, gh, r.left, r.top);
    config_.data().window_geometry = buf;

    // Save tab group geometry
    if (tab_group_) {
        tab_group_->save_geometry();
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
                // Refresh all control texts
                SetWindowTextW(hwnd_, t("app.title").c_str());
                SetWindowTextW(timeout_label_, t("settings.timeout").c_str());
                SetWindowTextW(timeout_unit_, t("settings.timeout_unit").c_str());
                if (history_) history_->refresh_texts();
                if (tab_group_) tab_group_->refresh_texts();
                LOG_INFO("main_window", "Language changed to %s",
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

    // Set wait cursor on the window class
    saved_cursor_ = (HCURSOR)SetClassLongPtrW(hwnd_, GCLP_HCURSOR,
        (LONG_PTR)LoadCursorW(nullptr, IDC_WAIT));
    SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    show_toast();

    // Block input while opening
    EnableWindow(hwnd_, FALSE);

    HWND main_hwnd = hwnd_;
    std::atomic<bool>* closing_ptr = &closing_;
    std::thread([valid, rect, timeout, main_hwnd, closing_ptr]() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        try {
            fto::open_folders_as_tabs(valid,
                nullptr, // on_progress
                [main_hwnd, closing_ptr](const std::wstring& p, const std::wstring& e) {
                    if (closing_ptr->load()) return;
                    PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                        (WPARAM)new std::wstring(p),
                        (LPARAM)new std::wstring(e));
                },
                timeout, rect);
        } catch (const std::exception& e) {
            if (!closing_ptr->load()) {
                std::wstring err = utf8_to_wide(e.what());
                PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                    (WPARAM)new std::wstring(L""),
                    (LPARAM)new std::wstring(err));
            }
        }
        CoUninitialize();
        if (!closing_ptr->load()) {
            PostMessageW(main_hwnd, WM_TAB_OPEN_COMPLETE, 0, 0);
        }
    }).detach();
}

void MainWindow::on_tab_open_complete() {
    EnableWindow(hwnd_, TRUE);
    hide_toast();
    // Restore cursor
    if (saved_cursor_) {
        SetClassLongPtrW(hwnd_, GCLP_HCURSOR, (LONG_PTR)saved_cursor_);
        saved_cursor_ = nullptr;
    }
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

    // Create toast font (13pt, matching Python version)
    if (!toast_font_) {
        HDC hdc = GetDC(hwnd_);
        int height = -MulDiv(13, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(hwnd_, hdc);
        toast_font_ = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }

    toast_hwnd_ = CreateWindowExW(0, L"STATIC",
        t("toast.opening_tabs").c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd_, nullptr, nullptr, nullptr);
    // Position in center
    int tw = client_w_ / 2, th = client_h_ / 2;
    MoveWindow(toast_hwnd_, (client_w_ - tw) / 2, (client_h_ - th) / 2, tw, th, TRUE);
    SendMessageW(toast_hwnd_, WM_SETFONT, (WPARAM)toast_font_, TRUE);
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

void MainWindow::update_dark_mode() {
    dark_mode_ = is_dark_mode();
    apply_dark_title_bar(hwnd_);

    // Recreate brushes
    if (dark_bg_brush_) { DeleteObject(dark_bg_brush_); dark_bg_brush_ = nullptr; }
    if (dark_edit_brush_) { DeleteObject(dark_edit_brush_); dark_edit_brush_ = nullptr; }

    if (dark_mode_) {
        dark_bg_brush_ = CreateSolidBrush(RGB(0x2B, 0x2B, 0x2B));
        dark_edit_brush_ = CreateSolidBrush(RGB(0x3C, 0x3C, 0x3C));
    }

    // Propagate dark mode to tab view
    if (tab_group_) {
        tab_group_->tab_view().set_dark_mode(dark_mode_);
    }

    InvalidateRect(hwnd_, nullptr, TRUE);
    LOG_INFO("main_window", "Dark mode: %s", dark_mode_ ? "on" : "off");
}

HBRUSH MainWindow::handle_ctlcolor(HDC hdc, HWND ctrl) {
    if (!dark_mode_) return nullptr;

    // Edit controls get a slightly lighter background
    wchar_t cls[64];
    GetClassNameW(ctrl, cls, 64);
    bool is_edit = (wcscmp(cls, L"Edit") == 0);
    bool is_listbox = (wcscmp(cls, L"ListBox") == 0);

    SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
    SetBkMode(hdc, TRANSPARENT);

    if (is_edit || is_listbox) {
        SetBkColor(hdc, RGB(0x3C, 0x3C, 0x3C));
        return dark_edit_brush_;
    }

    SetBkColor(hdc, RGB(0x2B, 0x2B, 0x2B));
    return dark_bg_brush_;
}

} // namespace fto

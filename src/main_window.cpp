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
#include <shlwapi.h>
#include <algorithm>

namespace fto {

// Layout constants (W5: extract magic numbers)
namespace layout {
    constexpr int PAD            = 10;   // Standard padding
    constexpr int ROW_H          = 28;   // Standard row height
    constexpr int COMBO_H        = 22;   // Combobox visible height
    constexpr int TOP_MARGIN     = 5;    // Settings bar top margin
    constexpr int SECTION_GAP    = 5;    // Gap between sections
    constexpr int SEPARATOR_H    = 2;    // Separator line height
    constexpr int SEPARATOR_GAP  = 7;    // Gap after separator
    constexpr int BOTTOM_MARGIN  = 5;    // Bottom padding
    constexpr int LANG_COMBO_W   = 100;  // Language combo width
    constexpr int LANG_LABEL_W   = 20;   // Language label (globe) width
    constexpr int TO_UNIT_W      = 30;   // Timeout unit label width
    constexpr int TO_COMBO_W     = 50;   // Timeout combo width
    constexpr int TO_LABEL_W     = 70;   // Timeout label width
    constexpr int CONTROL_GAP    = 3;    // Small gap between controls
    constexpr int SECTION_MARGIN = 10;   // Gap between setting groups
    constexpr int MIN_WINDOW_W   = 600;  // Minimum window width
    constexpr int MIN_WINDOW_H   = 400;  // Minimum window height
    constexpr int TOAST_PAD      = 40;   // Toast internal padding
    constexpr int TOAST_MIN_PATH_W = 60; // Min width for compact path
    constexpr int TOAST_FONT_PT  = 13;   // Toast font size (pt)
    constexpr int TOGGLE_BTN_W   = 70;   // Layout toggle button width
}

static const wchar_t* MAIN_WINDOW_CLASS = L"FileTabOpenerMainWindow";

// --- Toast overlay custom window class ---

static const wchar_t* TOAST_CLASS = L"FTOToast";
static bool g_toast_registered = false;

struct ToastPaintInfo {
    bool dark_mode;
    HFONT font;
};

static LRESULT CALLBACK ToastProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCCREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        break;
    }
    case WM_ERASEBKGND:
        return TRUE; // We fill the entire background in WM_PAINT
    case WM_PAINT: {
        auto* info = reinterpret_cast<ToastPaintInfo*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!info) break;

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        COLORREF bg     = info->dark_mode ? theme::TOAST_BG_DARK     : theme::TOAST_BG_LIGHT;
        COLORREF fg     = info->dark_mode ? theme::TOAST_FG_DARK     : theme::TOAST_FG_LIGHT;
        COLORREF border = info->dark_mode ? theme::TOAST_BORDER_DARK : theme::TOAST_BORDER_LIGHT;

        // Fill background
        HBRUSH bg_brush = CreateSolidBrush(bg);
        FillRect(hdc, &rc, bg_brush);
        DeleteObject(bg_brush);

        // Draw border (2px, matching Python highlightthickness=2)
        HPEN border_pen = CreatePen(PS_SOLID, 2, border);
        HPEN old_pen = (HPEN)SelectObject(hdc, border_pen);
        HBRUSH old_brush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
        DeleteObject(border_pen);

        // Draw text centered
        HFONT old_font = info->font ? (HFONT)SelectObject(hdc, info->font) : nullptr;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, fg);

        wchar_t text[512];
        GetWindowTextW(hwnd, text, 512);

        // Calculate text height for vertical centering
        RECT calc_rc = rc;
        InflateRect(&calc_rc, -16, 0);
        int text_h = DrawTextW(hdc, text, -1, &calc_rc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);

        // Draw vertically centered
        RECT draw_rc = rc;
        InflateRect(&draw_rc, -16, 0);
        int y_offset = ((rc.bottom - rc.top) - text_h) / 2;
        draw_rc.top = y_offset;
        draw_rc.bottom = y_offset + text_h;
        DrawTextW(hdc, text, -1, &draw_rc, DT_CENTER | DT_WORDBREAK);

        if (old_font) SelectObject(hdc, old_font);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NCDESTROY: {
        auto* info = reinterpret_cast<ToastPaintInfo*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        delete info;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

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
        // Handle Enter key in path entry (Compact and Sidebar)
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND focus = GetFocus();
            LONG id = GetWindowLongW(focus, GWL_ID);
            if (id == IDC_PATH_ENTRY && classic_tab_group_ && !use_modern_) {
                classic_tab_group_->handle_command(MAKEWPARAM(IDC_PATH_ADD_BTN, BN_CLICKED), 0);
                continue;
            }
            if (id == IDC_MODERN_PATH_ENTRY && modern_tab_group_ && use_modern_) {
                modern_tab_group_->handle_command(MAKEWPARAM(IDC_MODERN_ADD_BTN, BN_CLICKED), 0);
                continue;
            }
            if (id == IDC_MODERN_NEW_GROUP_ENTRY && modern_tab_group_ && use_modern_) {
                modern_tab_group_->handle_command(MAKEWPARAM(IDC_MODERN_NEW_GROUP_BTN, BN_CLICKED), 0);
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
        mmi->ptMinTrackSize.x = layout::MIN_WINDOW_W;
        mmi->ptMinTrackSize.y = layout::MIN_WINDOW_H;
        return 0;
    }
    case WM_COMMAND:
        self->on_command(wParam, lParam);
        return 0;
    case WM_NOTIFY:
        if (self->modern_tab_group_ && self->use_modern_) {
            if (self->modern_tab_group_->handle_notify(wParam, lParam))
                return 0;
        }
        break;
    case WM_CONTEXTMENU: {
        // Forward right-click on modern path list
        if (self->use_modern_ && self->modern_tab_group_) {
            HWND target = (HWND)wParam;
            LONG id = GetWindowLongW(target, GWL_ID);
            if (id == IDC_MODERN_PATH_LIST) {
                int sel = (int)SendMessageW(target, LB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU menu = CreatePopupMenu();
                    AppendMenuW(menu, MF_STRING, IDM_MODERN_PATH_REMOVE,
                        t("path.remove").c_str());
                    AppendMenuW(menu, MF_STRING, IDM_MODERN_PATH_OPEN_EXPLORER,
                        t("path.open_explorer").c_str());
                    AppendMenuW(menu, MF_STRING, IDM_MODERN_PATH_COPY_PATH,
                        t("path.copy_path").c_str());
                    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                    DestroyMenu(menu);
                }
                return 0;
            }
        }
        break;
    }
    case WM_TAB_OPEN_COMPLETE:
        self->on_tab_open_complete();
        return 0;
    case WM_TAB_OPEN_PROGRESS: {
        int current = LOWORD(wParam);
        int total = HIWORD(wParam);
        auto* path = reinterpret_cast<std::wstring*>(lParam);
        if (path) {
            self->update_toast(current, total, *path);
            delete path;
        }
        return 0;
    }
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
            COLORREF bg, pressed, text_color;
            COLORREF parent_bg = self->dark_mode_ ? theme::PARENT_BG_DARK : GetSysColor(COLOR_3DFACE);

            // Layout toggle buttons: selected state = pressed appearance
            if (dis->CtlID == IDC_LAYOUT_CLASSIC_BTN || dis->CtlID == IDC_LAYOUT_MODERN_BTN) {
                bool is_checked = (SendMessageW(dis->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (is_checked) {
                    bg = self->dark_mode_ ? theme::BTN_PRESSED_DARK : theme::BTN_PRESSED_LIGHT;
                    pressed = bg;
                } else {
                    bg = self->dark_mode_ ? theme::TAB_UNSEL_BG_DARK : theme::TAB_UNSEL_BG_LIGHT;
                    pressed = self->dark_mode_ ? theme::BTN_PRESSED_DARK : theme::BTN_PRESSED_LIGHT;
                }
                text_color = is_checked
                    ? (self->dark_mode_ ? theme::BTN_TEXT_DARK : theme::BTN_TEXT_LIGHT)
                    : (self->dark_mode_ ? theme::TAB_UNSEL_TEXT_DARK : theme::TAB_UNSEL_TEXT_LIGHT);
                draw_themed_button(dis, bg, pressed, text_color, parent_bg);
            } else {
                bg = self->dark_mode_ ? theme::BTN_BG_DARK : theme::BTN_BG_LIGHT;
                pressed = self->dark_mode_ ? theme::BTN_PRESSED_DARK : theme::BTN_PRESSED_LIGHT;
                text_color = self->dark_mode_ ? theme::BTN_TEXT_DARK : theme::BTN_TEXT_LIGHT;
                draw_themed_button(dis, bg, pressed, text_color, parent_bg);
            }
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

    // --- Layout toggle (left-aligned) ---
    layout_classic_btn_ = CreateWindowW(L"BUTTON", t("layout.compact").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_,
        (HMENU)(INT_PTR)IDC_LAYOUT_CLASSIC_BTN, nullptr, nullptr);
    SendMessageW(layout_classic_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    layout_modern_btn_ = CreateWindowW(L"BUTTON", t("layout.sidebar").c_str(),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_PUSHLIKE | BS_OWNERDRAW,
        0, 0, 0, 0, hwnd_,
        (HMENU)(INT_PTR)IDC_LAYOUT_MODERN_BTN, nullptr, nullptr);
    SendMessageW(layout_modern_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Read saved layout preference
    auto layout_val = config_.get_setting("layout", "compact");
    use_modern_ = (layout_val == "sidebar" || layout_val == "modern"); // backward compat
    SendMessageW(use_modern_ ? layout_modern_btn_ : layout_classic_btn_,
        BM_SETCHECK, BST_CHECKED, 0);

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

    // Tab group sections (Compact and Sidebar)
    auto open_cb = [this](const std::vector<std::wstring>& paths,
                          const std::optional<WindowRect>& rect) {
        open_folders_as_tabs(paths, rect);
    };
    classic_tab_group_ = std::make_unique<TabGroupSection>(config_, open_cb);
    modern_tab_group_ = std::make_unique<ModernTabGroupSection>(config_, open_cb);

    // Initialize dark mode
    update_dark_mode();

    // Initial layout will happen in WM_SIZE
}

void MainWindow::on_size(int w, int h) {
    using namespace layout;
    client_w_ = w;
    client_h_ = h;
    int cy = TOP_MARGIN;

    // Layout toggle (left-aligned)
    MoveWindow(layout_classic_btn_, PAD, cy, TOGGLE_BTN_W, ROW_H, TRUE);
    MoveWindow(layout_modern_btn_, PAD + TOGGLE_BTN_W, cy, TOGGLE_BTN_W, ROW_H, TRUE);

    // Settings bar (right-aligned)
    int right = w - PAD;

    // Language combo (rightmost)
    MoveWindow(lang_combo_, right - LANG_COMBO_W, cy, LANG_COMBO_W, COMBO_H + 100, TRUE);
    right -= LANG_COMBO_W + CONTROL_GAP;
    MoveWindow(lang_label_, right - LANG_LABEL_W, cy, LANG_LABEL_W, ROW_H, TRUE);
    right -= LANG_LABEL_W + SECTION_MARGIN;

    // Timeout
    MoveWindow(timeout_unit_, right - TO_UNIT_W, cy, TO_UNIT_W, ROW_H, TRUE);
    right -= TO_UNIT_W + 1;
    MoveWindow(timeout_combo_, right - TO_COMBO_W, cy, TO_COMBO_W, COMBO_H + 100, TRUE);
    right -= TO_COMBO_W + CONTROL_GAP;
    MoveWindow(timeout_label_, right - TO_LABEL_W, cy, TO_LABEL_W, ROW_H, TRUE);

    cy += ROW_H + SECTION_GAP;

    // History section
    if (history_) {
        if (!history_->is_created()) {
            history_->create(hwnd_, PAD, cy, w - PAD * 2, ROW_H);
        } else {
            history_->resize(PAD, cy, w - PAD * 2, ROW_H);
        }
    }
    cy += ROW_H + SECTION_GAP;

    // Separator
    MoveWindow(separator_, PAD, cy, w - PAD * 2, SEPARATOR_H, TRUE);
    cy += SEPARATOR_GAP;

    // Tab group section (fills the rest)
    int remaining = h - cy - BOTTOM_MARGIN;

    // Compact layout
    if (classic_tab_group_) {
        if (!classic_tab_group_->is_created()) {
            classic_tab_group_->create(hwnd_, PAD, cy, w - PAD * 2, remaining);
            if (use_modern_) classic_tab_group_->show(false);
        } else {
            classic_tab_group_->resize(PAD, cy, w - PAD * 2, remaining);
        }
    }

    // Sidebar layout
    if (modern_tab_group_) {
        if (!modern_tab_group_->is_created()) {
            modern_tab_group_->create(hwnd_, PAD, cy, w - PAD * 2, remaining);
            if (!use_modern_) modern_tab_group_->show(false);
        } else {
            modern_tab_group_->resize(PAD, cy, w - PAD * 2, remaining);
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
    join_open_thread();  // wait for background thread before cleanup

    // Save window geometry
    RECT r;
    GetWindowRect(hwnd_, &r);
    int gw = r.right - r.left, gh = r.bottom - r.top;
    char buf[64];
    snprintf(buf, sizeof(buf), "%dx%d+%d+%d", gw, gh, r.left, r.top);
    config_.data().window_geometry = buf;

    // Save tab group geometry
    auto* active = active_tab_group();
    if (active) active->save_geometry();

    config_.save();
    LOG_INFO("main_window", "Window closed (geometry: %s)", buf);
    DestroyWindow(hwnd_);
}

void MainWindow::on_command(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);

    // Layout toggle
    if (id == IDC_LAYOUT_CLASSIC_BTN && HIWORD(wParam) == BN_CLICKED) {
        if (use_modern_) switch_layout(false);
        return;
    }
    if (id == IDC_LAYOUT_MODERN_BTN && HIWORD(wParam) == BN_CLICKED) {
        if (!use_modern_) switch_layout(true);
        return;
    }

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
                SetWindowTextW(layout_classic_btn_, t("layout.compact").c_str());
                SetWindowTextW(layout_modern_btn_, t("layout.sidebar").c_str());
                if (history_) history_->refresh_texts();
                if (classic_tab_group_) classic_tab_group_->refresh_texts();
                if (modern_tab_group_) modern_tab_group_->refresh_texts();
                // Force repaint of toggle buttons
                InvalidateRect(layout_classic_btn_, nullptr, TRUE);
                InvalidateRect(layout_modern_btn_, nullptr, TRUE);
                LOG_INFO("main_window", "Language changed to %s",
                         wide_to_utf8(code).c_str());
            }
        }
        return;
    }

    // Forward to active section
    if (history_ && history_->handle_command(wParam, lParam)) return;
    if (use_modern_) {
        if (modern_tab_group_ && modern_tab_group_->handle_command(wParam, lParam)) return;
    } else {
        if (classic_tab_group_ && classic_tab_group_->handle_command(wParam, lParam)) return;
    }
}

void MainWindow::switch_layout(bool use_modern) {
    // Save current state
    auto* old_section = active_tab_group();
    std::wstring current_name;
    if (old_section) {
        old_section->save_geometry();
        current_name = old_section->current_tab_name();
    }

    // Toggle
    use_modern_ = use_modern;
    config_.set_setting("layout", use_modern ? "sidebar" : "compact");
    config_.save();

    // Show/hide
    if (classic_tab_group_) classic_tab_group_->show(!use_modern);
    if (modern_tab_group_) modern_tab_group_->show(use_modern);

    // Transfer selection to new section
    auto* new_section = active_tab_group();
    if (new_section && !current_name.empty()) {
        new_section->select_tab(current_name);
    }

    // Force repaint of toggle buttons
    InvalidateRect(layout_classic_btn_, nullptr, TRUE);
    InvalidateRect(layout_modern_btn_, nullptr, TRUE);

    LOG_INFO("main_window", "Layout switched to %s", use_modern ? "sidebar" : "compact");
}

ITabGroupUI* MainWindow::active_tab_group() {
    if (use_modern_) return modern_tab_group_.get();
    return classic_tab_group_.get();
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

    // Disable input BEFORE showing toast — EnableWindow triggers child
    // control repaints, which would flash over the toast if done after.
    EnableWindow(hwnd_, FALSE);
    show_toast((int)valid.size(), valid[0]);

    HWND main_hwnd = hwnd_;
    std::atomic<bool>* closing_ptr = &closing_;
    join_open_thread();  // ensure previous thread is done
    open_thread_ = std::thread([valid, rect, timeout, main_hwnd, closing_ptr]() {
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        try {
            if (!closing_ptr->load()) {
                fto::open_folders_as_tabs(valid,
                    [main_hwnd, closing_ptr](int current, int total, const std::wstring& path) {
                        if (closing_ptr->load()) return;
                        auto* ps = new std::wstring(path);
                        if (!PostMessageW(main_hwnd, WM_TAB_OPEN_PROGRESS,
                                MAKEWPARAM(current, total), (LPARAM)ps)) {
                            delete ps;
                        }
                    },
                    [main_hwnd, closing_ptr](const std::wstring& p, const std::wstring& e) {
                        if (closing_ptr->load()) return;
                        auto* pp = new std::wstring(p);
                        auto* pe = new std::wstring(e);
                        if (!PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                                (WPARAM)pp, (LPARAM)pe)) {
                            delete pp;
                            delete pe;
                        }
                    },
                    timeout, rect);
            }
        } catch (const std::exception& e) {
            if (!closing_ptr->load()) {
                auto* pp = new std::wstring(L"");
                auto* pe = new std::wstring(utf8_to_wide(e.what()));
                if (!PostMessageW(main_hwnd, WM_TAB_OPEN_ERROR,
                        (WPARAM)pp, (LPARAM)pe)) {
                    delete pp;
                    delete pe;
                }
            }
        }
        CoUninitialize();
        if (!closing_ptr->load()) {
            PostMessageW(main_hwnd, WM_TAB_OPEN_COMPLETE, 0, 0);
        }
    });
}

void MainWindow::on_tab_open_complete() {
    join_open_thread();
    EnableWindow(hwnd_, TRUE);
    hide_toast();
    // Restore cursor
    if (saved_cursor_) {
        SetClassLongPtrW(hwnd_, GCLP_HCURSOR, (LONG_PTR)saved_cursor_);
        saved_cursor_ = nullptr;
    }
    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
    auto* active = active_tab_group();
    if (active) active->set_opening(false);
    LOG_INFO("main_window", "Tab opening complete");
}

void MainWindow::on_tab_open_error(const std::wstring& path, const std::wstring& error) {
    MessageBoxW(hwnd_,
        t("error.open_failed", {{"path", path}, {"error", error}}).c_str(),
        t("error.title").c_str(), MB_ICONERROR);
}

void MainWindow::show_toast(int total, const std::wstring& first_path) {
    hide_toast();

    // Create toast font (13pt, system font face)
    if (!toast_font_) {
        NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        HDC hdc = GetDC(hwnd_);
        ncm.lfMessageFont.lfHeight = -MulDiv(layout::TOAST_FONT_PT, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(hwnd_, hdc);
        toast_font_ = CreateFontIndirectW(&ncm.lfMessageFont);
    }

    // Register toast class once
    if (!g_toast_registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = ToastProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = TOAST_CLASS;
        wc.hCursor = LoadCursorW(nullptr, IDC_WAIT);
        RegisterClassW(&wc);
        g_toast_registered = true;
    }

    // Initial text (1/N, first path)
    std::wstring text = build_toast_text(1, total, first_path);

    auto* paint_info = new ToastPaintInfo{dark_mode_, toast_font_};
    int tw = client_w_ / 2, th = client_h_ / 2;
    // Create hidden with WS_CLIPSIBLINGS to prevent sibling controls
    // from painting over the toast area, then show + force immediate paint.
    toast_hwnd_ = CreateWindowExW(0, TOAST_CLASS, text.c_str(),
        WS_CHILD | WS_CLIPSIBLINGS,
        (client_w_ - tw) / 2, (client_h_ - th) / 2, tw, th,
        hwnd_, nullptr, GetModuleHandleW(nullptr), paint_info);

    SetWindowPos(toast_hwnd_, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow(toast_hwnd_);  // Force synchronous WM_PAINT before returning
    LOG_INFO("toast", "Toast shown (total=%d)", total);
}

void MainWindow::update_toast(int current, int total, const std::wstring& path) {
    if (!toast_hwnd_ || !IsWindow(toast_hwnd_)) return;

    std::wstring text = build_toast_text(current, total, path);
    SetWindowTextW(toast_hwnd_, text.c_str());
    InvalidateRect(toast_hwnd_, nullptr, TRUE);
    LOG_DEBUG("toast", "Progress %d/%d: %s", current, total, wide_to_utf8(path).c_str());
}

std::wstring MainWindow::build_toast_text(int current, int total, const std::wstring& path) {
    std::wstring header = t("toast.progress_header", {
        {"current", std::to_wstring(current)},
        {"total",   std::to_wstring(total)}
    });
    std::wstring footer = t("toast.wait_message");

    // Compact path with PathCompactPathW (Windows standard ellipsis)
    std::wstring path_line;
    if (!path.empty()) {
        int toast_w = client_w_ / 2;
        int available_w = std::max(toast_w - layout::TOAST_PAD, layout::TOAST_MIN_PATH_W);
        wchar_t compact[MAX_PATH];
        wcscpy_s(compact, path.c_str());
        HDC hdc = GetDC(toast_hwnd_ ? toast_hwnd_ : hwnd_);
        HFONT old_font = toast_font_ ? (HFONT)SelectObject(hdc, toast_font_) : nullptr;
        PathCompactPathW(hdc, compact, available_w);
        if (old_font) SelectObject(hdc, old_font);
        ReleaseDC(toast_hwnd_ ? toast_hwnd_ : hwnd_, hdc);
        path_line = compact;
    }

    return header + L"\n" + path_line + L"\n" + footer;
}

void MainWindow::hide_toast() {
    if (toast_hwnd_ && IsWindow(toast_hwnd_)) {
        DestroyWindow(toast_hwnd_);
        toast_hwnd_ = nullptr;
        LOG_DEBUG("toast", "Toast hidden");
    }
}

void MainWindow::join_open_thread() {
    if (open_thread_.joinable()) {
        open_thread_.join();
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

    // Propagate dark mode to both sections
    if (classic_tab_group_) classic_tab_group_->set_dark_mode(dark_mode_);
    if (modern_tab_group_) modern_tab_group_->set_dark_mode(dark_mode_);

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

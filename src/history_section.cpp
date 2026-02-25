#include "history_section.h"
#include "i18n.h"
#include "utils.h"
#include "logger.h"
#include "res/resource.h"
#include <shlwapi.h>
#include <commctrl.h>

namespace fto {

static const wchar_t* DROPDOWN_CLASS = L"FTOHistoryDropdown";
static bool g_dropdown_registered = false;

static LRESULT CALLBACK DropdownProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) {
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HistorySection::HistorySection(ConfigManager& config, OpenFolderCallback on_open)
    : config_(config), on_open_folder_(on_open) {
    if (!g_dropdown_registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = DropdownProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = DROPDOWN_CLASS;
        wc.style = CS_DROPSHADOW;
        RegisterClassW(&wc);
        g_dropdown_registered = true;
    }
}

void HistorySection::create(HWND parent, int x, int y, int w, int h) {
    parent_ = parent;
    x_ = x; y_ = y; w_ = w; h_ = h;
    font_ = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    int cx = x, btn_w = 80, small_btn_w = 50;

    // Label
    label_ = CreateWindowW(L"STATIC", t("history.label").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        cx, y, 55, h, parent, (HMENU)(INT_PTR)IDC_HISTORY_LABEL, nullptr, nullptr);
    SendMessageW(label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += 55;

    // Buttons from right
    int right_x = x + w;
    clear_btn_ = CreateWindowW(L"BUTTON", t("history.clear").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        right_x - small_btn_w, y, small_btn_w, h, parent,
        (HMENU)(INT_PTR)IDC_HISTORY_CLEAR_BTN, nullptr, nullptr);
    SendMessageW(clear_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    right_x -= small_btn_w + 3;

    pin_btn_ = CreateWindowW(L"BUTTON", t("history.pin").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        right_x - small_btn_w, y, small_btn_w, h, parent,
        (HMENU)(INT_PTR)IDC_HISTORY_PIN_BTN, nullptr, nullptr);
    SendMessageW(pin_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    right_x -= small_btn_w + 3;

    open_btn_ = CreateWindowW(L"BUTTON", t("history.open_explorer").c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        right_x - 130, y, 130, h, parent,
        (HMENU)(INT_PTR)IDC_HISTORY_OPEN_BTN, nullptr, nullptr);
    SendMessageW(open_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    right_x -= 130 + 3;

    // Dropdown button
    int dd_w = 24;
    dropdown_btn_ = CreateWindowW(L"BUTTON", L"\x25BC",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        right_x - dd_w, y, dd_w, h, parent,
        (HMENU)(INT_PTR)IDC_HISTORY_DROPDOWN_BTN, nullptr, nullptr);
    SendMessageW(dropdown_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    right_x -= dd_w;

    // Entry (fills remaining space)
    int entry_w = right_x - cx - 3;
    entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        cx, y + 2, entry_w, h - 4, parent,
        (HMENU)(INT_PTR)IDC_HISTORY_ENTRY, nullptr, nullptr);
    SendMessageW(entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    SendMessageW(entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());
}

void HistorySection::resize(int x, int y, int w, int h) {
    x_ = x; y_ = y; w_ = w; h_ = h;
    // Recalculate positions
    int cx = x, btn_w = 80, small_btn_w = 50;

    MoveWindow(label_, cx, y, 55, h, TRUE);
    cx += 55;

    int right_x = x + w;
    MoveWindow(clear_btn_, right_x - small_btn_w, y, small_btn_w, h, TRUE);
    right_x -= small_btn_w + 3;
    MoveWindow(pin_btn_, right_x - small_btn_w, y, small_btn_w, h, TRUE);
    right_x -= small_btn_w + 3;
    MoveWindow(open_btn_, right_x - 130, y, 130, h, TRUE);
    right_x -= 130 + 3;
    int dd_w = 24;
    MoveWindow(dropdown_btn_, right_x - dd_w, y, dd_w, h, TRUE);
    right_x -= dd_w;
    int entry_w = right_x - cx - 3;
    MoveWindow(entry_, cx, y + 2, entry_w, h - 4, TRUE);
}

bool HistorySection::handle_command(WPARAM wParam, LPARAM lParam) {
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);

    switch (id) {
    case IDC_HISTORY_OPEN_BTN: on_open(); return true;
    case IDC_HISTORY_PIN_BTN: on_pin(); return true;
    case IDC_HISTORY_CLEAR_BTN: on_clear(); return true;
    case IDC_HISTORY_DROPDOWN_BTN: toggle_dropdown(); return true;
    case IDC_HISTORY_ENTRY:
        if (code == EN_KILLFOCUS) close_dropdown();
        return false;
    }
    return false;
}

std::wstring HistorySection::get_entry_text() {
    wchar_t buf[2048];
    GetWindowTextW(entry_, buf, 2048);
    std::wstring text(buf);
    // Strip pin prefix
    if (text.size() >= 3 && text[0] == 0xD83D && text[1] == 0xDCCC) {
        text = text.substr(3); // 📌 + space
    } else if (text.size() >= 3 && text[0] == L' ' && text[1] == L' ' && text[2] == L' ') {
        text = text.substr(3);
    }
    // Trim
    while (!text.empty() && iswspace(text.front())) text.erase(text.begin());
    while (!text.empty() && iswspace(text.back())) text.pop_back();
    return text;
}

void HistorySection::set_entry_text(const std::wstring& text) {
    SetWindowTextW(entry_, text.c_str());
}

void HistorySection::on_open() {
    std::wstring path = strip_quotes(get_entry_text());
    if (path.empty()) return;

    std::wstring expanded = expand_user(path);
    if (is_unc_path(expanded) || is_directory(expanded)) {
        LOG_INFO("history", "Opening: %s", wide_to_utf8(expanded).c_str());
        config_.add_history(expanded);
        config_.save();
        set_entry_text(expanded);
        on_open_folder_(expanded);
    } else {
        MessageBoxW(parent_, t("history.invalid_path_msg", {{"path", path}}).c_str(),
                     t("history.invalid_path_title").c_str(), MB_ICONWARNING);
    }
}

void HistorySection::on_pin() {
    std::wstring path = get_entry_text();
    if (path.empty()) return;
    std::wstring expanded = expand_user(path);
    std::wstring normalized = normalize_path(expanded);

    // Ensure it's in history
    bool found = false;
    for (const auto& e : config_.data().history) {
        if (normalize_path(e.path) == normalized) { found = true; break; }
    }
    if (!found) config_.add_history(expanded);
    config_.toggle_pin(normalized);
    config_.save();
    LOG_INFO("history", "Pin toggled: %s", wide_to_utf8(normalized).c_str());
}

void HistorySection::on_clear() {
    int result = MessageBoxW(parent_,
        t("history.clear_confirm_msg").c_str(),
        t("history.clear_confirm_title").c_str(),
        MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        config_.clear_history(true);
        config_.save();
        LOG_INFO("history", "History cleared");
    }
}

void HistorySection::toggle_dropdown() {
    if (dropdown_popup_) {
        close_dropdown();
        return;
    }
    show_dropdown();
}

void HistorySection::show_dropdown() {
    auto history = config_.get_sorted_history();
    if (history.empty()) return;

    RECT entry_rect;
    GetWindowRect(entry_, &entry_rect);
    int popup_w = entry_rect.right - entry_rect.left + 24;
    int item_h = 20;
    int max_items = std::min((int)history.size(), 10);
    int popup_h = max_items * item_h + 4;

    dropdown_popup_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        DROPDOWN_CLASS, L"",
        WS_POPUP | WS_VISIBLE | WS_BORDER,
        entry_rect.left, entry_rect.bottom,
        popup_w, popup_h,
        parent_, nullptr, GetModuleHandleW(nullptr), nullptr);

    dropdown_list_ = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        0, 0, popup_w, popup_h,
        dropdown_popup_, (HMENU)9999, nullptr, nullptr);

    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessageW(dropdown_list_, WM_SETFONT, (WPARAM)font, TRUE);

    for (const auto& e : history) {
        std::wstring prefix = e.pinned ? L"\xD83D\xDCCC " : L"   ";
        std::wstring item = prefix + e.path;
        SendMessageW(dropdown_list_, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }

    // Subclass the dropdown popup to handle listbox commands
    static constexpr UINT_PTR DROPDOWN_SUBCLASS_ID = 1;
    SetWindowSubclass(dropdown_popup_, [](
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) -> LRESULT
    {
        auto* self = reinterpret_cast<HistorySection*>(dwRefData);
        if (msg == WM_COMMAND && HIWORD(wParam) == LBN_SELCHANGE && self) {
            int sel = (int)SendMessageW(self->dropdown_list_, LB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                int len = (int)SendMessageW(self->dropdown_list_, LB_GETTEXTLEN, sel, 0);
                std::wstring text(len, L'\0');
                SendMessageW(self->dropdown_list_, LB_GETTEXT, sel, (LPARAM)text.data());
                // Strip prefix
                if (text.size() >= 3 && text[0] == 0xD83D) text = text.substr(3);
                else if (text.size() >= 3 && text[0] == L' ') text = text.substr(3);
                while (!text.empty() && iswspace(text.front())) text.erase(text.begin());
                self->set_entry_text(text);
                self->close_dropdown();
            }
            return 0;
        }
        if (msg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) {
            DestroyWindow(hwnd);
            if (self) {
                self->dropdown_popup_ = nullptr;
                self->dropdown_list_ = nullptr;
            }
            return 0;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }, DROPDOWN_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));

    SetFocus(dropdown_list_);
}

void HistorySection::close_dropdown() {
    if (dropdown_popup_) {
        DestroyWindow(dropdown_popup_);
        dropdown_popup_ = nullptr;
        dropdown_list_ = nullptr;
    }
}

void HistorySection::refresh_texts() {
    SetWindowTextW(label_, t("history.label").c_str());
    SetWindowTextW(open_btn_, t("history.open_explorer").c_str());
    SetWindowTextW(pin_btn_, t("history.pin").c_str());
    SetWindowTextW(clear_btn_, t("history.clear").c_str());
    SendMessageW(entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());
}

} // namespace fto

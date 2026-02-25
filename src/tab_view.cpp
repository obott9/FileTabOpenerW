#include "tab_view.h"
#include "theme.h"
#include "logger.h"
#include "utils.h"
#include <algorithm>

namespace fto {

static const wchar_t* TAB_VIEW_CLASS = L"FTOTabView";

void TabView::register_class() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = TAB_VIEW_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);
}

void TabView::create(HWND parent, int x, int y, int w, int h, TabChangedCallback cb) {
    parent_ = parent;
    on_tab_changed_ = cb;
    font_ = create_system_message_font();

    hwnd_ = CreateWindowExW(0, TAB_VIEW_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL,
        x, y, w, h, parent, nullptr, GetModuleHandleW(nullptr), this);
}

void TabView::resize(int x, int y, int w, int h) {
    if (hwnd_) {
        MoveWindow(hwnd_, x, y, w, h, TRUE);
        layout();
    }
}

LRESULT CALLBACK TabView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TabView* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<TabView*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<TabView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_SIZE:
        self->layout();
        return 0;
    case WM_VSCROLL: {
        SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(hwnd, SB_VERT, &si);
        int old_pos = si.nPos;
        switch (LOWORD(wParam)) {
        case SB_LINEUP:    si.nPos -= BTN_HEIGHT; break;
        case SB_LINEDOWN:  si.nPos += BTN_HEIGHT; break;
        case SB_PAGEUP:    si.nPos -= si.nPage; break;
        case SB_PAGEDOWN:  si.nPos += si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        }
        si.nPos = std::clamp(si.nPos, si.nMin, std::max(si.nMax - (int)si.nPage + 1, 0));
        if (si.nPos != old_pos) {
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            self->scroll_pos_ = si.nPos;
            self->layout();
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(hwnd, SB_VERT, &si);
        si.nPos -= (delta / 120) * BTN_HEIGHT;
        si.nPos = std::clamp(si.nPos, si.nMin, std::max(si.nMax - (int)si.nPage + 1, 0));
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        self->scroll_pos_ = si.nPos;
        self->layout();
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            self->on_button_click(LOWORD(wParam));
        }
        return 0;
    case WM_DRAWITEM: {
        auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (dis->CtlType == ODT_BUTTON) {
            self->draw_tab_button(dis);
            return TRUE;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void TabView::add_tab(const std::wstring& name) {
    if (std::find(names_.begin(), names_.end(), name) != names_.end()) return;
    names_.push_back(name);
    if (current_.empty()) current_ = name;
    layout();
}

void TabView::delete_tab(const std::wstring& name) {
    auto it = std::find(names_.begin(), names_.end(), name);
    if (it == names_.end()) return;
    int idx = (int)(it - names_.begin());
    names_.erase(it);
    if (current_ == name) {
        if (!names_.empty()) {
            int new_idx = std::min(idx, (int)names_.size() - 1);
            current_ = names_[new_idx];
        } else {
            current_.clear();
        }
    }
    layout();
}

void TabView::rename_tab(const std::wstring& old_name, const std::wstring& new_name) {
    auto it = std::find(names_.begin(), names_.end(), old_name);
    if (it == names_.end()) return;
    *it = new_name;
    if (current_ == old_name) current_ = new_name;
    layout();
}

void TabView::set_current_tab(const std::wstring& name) {
    if (std::find(names_.begin(), names_.end(), name) == names_.end()) return;
    current_ = name;
    update_selection();
}

void TabView::move_tab(int old_index, int new_index) {
    if (old_index < 0 || old_index >= (int)names_.size()) return;
    if (new_index < 0 || new_index >= (int)names_.size()) return;
    auto item = std::move(names_[old_index]);
    names_.erase(names_.begin() + old_index);
    names_.insert(names_.begin() + new_index, std::move(item));
    layout();
}

static std::wstring escape_ampersand(const std::wstring& s) {
    std::wstring result;
    result.reserve(s.size());
    for (wchar_t c : s) {
        result += c;
        if (c == L'&') result += L'&';
    }
    return result;
}

void TabView::layout() {
    if (!hwnd_) return;
    if (in_layout_) return;
    in_layout_ = true;

    // Destroy old buttons
    for (HWND btn : buttons_) DestroyWindow(btn);
    buttons_.clear();

    if (names_.empty()) {
        SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
        si.nMin = 0; si.nMax = 0; si.nPage = 1;
        SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
        in_layout_ = false;
        return;
    }

    RECT rc;
    GetClientRect(hwnd_, &rc);
    int available_w = rc.right - rc.left;

    // Measure button widths
    HDC hdc = GetDC(hwnd_);
    HFONT old_font = (HFONT)SelectObject(hdc, font_);
    struct BtnInfo { std::wstring name; int width; };
    std::vector<BtnInfo> infos;
    for (const auto& name : names_) {
        SIZE sz;
        GetTextExtentPoint32W(hdc, name.c_str(), (int)name.size(), &sz);
        int w = sz.cx + 20; // padding
        infos.push_back({name, std::max(w, 50)});
    }
    SelectObject(hdc, old_font);
    ReleaseDC(hwnd_, hdc);

    // Layout into rows
    struct Row { std::vector<int> indices; };
    std::vector<Row> rows;
    Row current_row;
    int row_used = 0;
    for (int i = 0; i < (int)infos.size(); ++i) {
        int needed = infos[i].width + BTN_PAD_X * 2;
        if (!current_row.indices.empty() && row_used + needed > available_w) {
            rows.push_back(std::move(current_row));
            current_row = {};
            row_used = 0;
        }
        current_row.indices.push_back(i);
        row_used += needed;
    }
    if (!current_row.indices.empty()) rows.push_back(std::move(current_row));

    int row_h = BTN_HEIGHT + BTN_PAD_Y * 2;
    total_height_ = (int)rows.size() * row_h;

    // Update scrollbar
    int visible_h = rc.bottom - rc.top;
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
    si.nMin = 0;
    si.nMax = total_height_ - 1;
    si.nPage = visible_h;
    si.nPos = std::clamp(scroll_pos_, 0, std::max(total_height_ - visible_h, 0));
    SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
    scroll_pos_ = si.nPos;

    // Create buttons
    int y_offset = -scroll_pos_;
    for (const auto& row : rows) {
        int x = BTN_PAD_X;
        for (int idx : row.indices) {
            std::wstring label = escape_ampersand(infos[idx].name);
            HWND btn = CreateWindowW(L"BUTTON", label.c_str(),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                x, y_offset + BTN_PAD_Y, infos[idx].width, BTN_HEIGHT,
                hwnd_, (HMENU)(INT_PTR)(BASE_BTN_ID + idx),
                GetModuleHandleW(nullptr), nullptr);
            SendMessageW(btn, WM_SETFONT, (WPARAM)font_, TRUE);
            buttons_.push_back(btn);
            x += infos[idx].width + BTN_PAD_X * 2;
        }
        y_offset += row_h;
    }

    in_layout_ = false;
    update_selection();
    in_layout_ = false;
}

void TabView::update_selection() {
    for (HWND btn : buttons_) {
        InvalidateRect(btn, nullptr, TRUE);
    }
}

void TabView::draw_tab_button(const DRAWITEMSTRUCT* dis) {
    int idx = (int)dis->CtlID - BASE_BTN_ID;
    bool selected = (idx >= 0 && idx < (int)names_.size() && names_[idx] == current_);

    COLORREF bg, pressed, text;
    if (selected) {
        bg      = dark_mode_ ? theme::BTN_BG_DARK      : theme::BTN_BG_LIGHT;
        pressed = dark_mode_ ? theme::BTN_PRESSED_DARK  : theme::BTN_PRESSED_LIGHT;
        text    = dark_mode_ ? theme::BTN_TEXT_DARK      : theme::BTN_TEXT_LIGHT;
    } else {
        bg      = dark_mode_ ? theme::TAB_UNSEL_BG_DARK      : theme::TAB_UNSEL_BG_LIGHT;
        pressed = dark_mode_ ? theme::TAB_UNSEL_PRESSED_DARK  : theme::TAB_UNSEL_PRESSED_LIGHT;
        text    = dark_mode_ ? theme::TAB_UNSEL_TEXT_DARK      : theme::TAB_UNSEL_TEXT_LIGHT;
    }

    COLORREF parent_bg = dark_mode_ ? theme::PARENT_BG_DARK : GetSysColor(COLOR_3DFACE);
    draw_themed_button(dis, bg, pressed, text, parent_bg);
}

void TabView::on_button_click(int btn_id) {
    int idx = btn_id - BASE_BTN_ID;
    if (idx >= 0 && idx < (int)names_.size()) {
        current_ = names_[idx];
        update_selection();
        if (on_tab_changed_) on_tab_changed_(current_);
    }
}

} // namespace fto

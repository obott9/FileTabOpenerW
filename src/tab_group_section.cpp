#include "tab_group_section.h"
#include "explorer_opener.h"
#include "input_dialog.h"
#include "theme.h"
#include "i18n.h"
#include "utils.h"
#include "logger.h"
#include "res/resource.h"
#include <shobjidl.h>
#include <objbase.h>
#include <wrl/client.h>
#include <commctrl.h>
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace fto {

TabGroupSection::TabGroupSection(ConfigManager& config, OpenTabsCallback on_open_tabs)
    : config_(config), on_open_tabs_(on_open_tabs) {}

void TabGroupSection::create(HWND parent, int x, int y, int w, int h) {
    parent_ = parent;
    x_ = x; y_ = y; w_ = w; h_ = h;
    font_ = create_system_message_font();

    int cx = x, cy = y;
    int btn_w = 85, btn_h = 26, small_btn = 30, pad = 3;

    // --- Tab management bar ---
    add_btn_ = CreateWindowW(L"BUTTON", t("tab.add").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_ADD_BTN, nullptr, nullptr);
    SendMessageW(add_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += btn_w + pad;

    del_btn_ = CreateWindowW(L"BUTTON", t("tab.delete").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_DELETE_BTN, nullptr, nullptr);
    SendMessageW(del_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += btn_w + pad;

    rename_btn_ = CreateWindowW(L"BUTTON", t("tab.rename").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_RENAME_BTN, nullptr, nullptr);
    SendMessageW(rename_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += btn_w + pad;

    copy_btn_ = CreateWindowW(L"BUTTON", t("tab.copy").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_COPY_BTN, nullptr, nullptr);
    SendMessageW(copy_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += btn_w + 10;

    left_btn_ = CreateWindowW(L"BUTTON", t("tab.move_left").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, small_btn, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_MOVE_LEFT_BTN, nullptr, nullptr);
    SendMessageW(left_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += small_btn + pad;

    right_btn_ = CreateWindowW(L"BUTTON", t("tab.move_right").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, small_btn, btn_h, parent,
        (HMENU)(INT_PTR)IDC_TAB_MOVE_RIGHT_BTN, nullptr, nullptr);
    SendMessageW(right_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    cy += btn_h + 5;

    // --- Tab view ---
    int tab_view_h = TabView::VISIBLE_ROWS * (TabView::BTN_HEIGHT + TabView::BTN_PAD_Y * 2);
    tab_view_.create(parent, x, cy, w, tab_view_h,
        [this](const std::wstring& name) { on_tab_changed(name); });
    cy += tab_view_h + 5;

    // --- Geometry inputs ---
    int geom_entry_w = 55, geom_label_w = 40;
    cx = x;

    geom_x_label_ = CreateWindowW(L"STATIC", t("window.x").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, cy, 20, btn_h, parent, nullptr, nullptr, nullptr);
    SendMessageW(geom_x_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += 20;
    geom_x_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, cy, geom_entry_w, btn_h - 2, parent,
        (HMENU)(INT_PTR)IDC_GEOM_X_ENTRY, nullptr, nullptr);
    SendMessageW(geom_x_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_y_label_ = CreateWindowW(L"STATIC", t("window.y").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, cy, 20, btn_h, parent, nullptr, nullptr, nullptr);
    SendMessageW(geom_y_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += 20;
    geom_y_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, cy, geom_entry_w, btn_h - 2, parent,
        (HMENU)(INT_PTR)IDC_GEOM_Y_ENTRY, nullptr, nullptr);
    SendMessageW(geom_y_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_w_label_ = CreateWindowW(L"STATIC", t("window.width").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, cy, geom_label_w, btn_h, parent, nullptr, nullptr, nullptr);
    SendMessageW(geom_w_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_label_w;
    geom_w_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, cy, geom_entry_w, btn_h - 2, parent,
        (HMENU)(INT_PTR)IDC_GEOM_W_ENTRY, nullptr, nullptr);
    SendMessageW(geom_w_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_h_label_ = CreateWindowW(L"STATIC", t("window.height").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, cy, geom_label_w, btn_h, parent, nullptr, nullptr, nullptr);
    SendMessageW(geom_h_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_label_w;
    geom_h_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, cy, geom_entry_w, btn_h - 2, parent,
        (HMENU)(INT_PTR)IDC_GEOM_H_ENTRY, nullptr, nullptr);
    SendMessageW(geom_h_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_get_btn_ = CreateWindowW(L"BUTTON", t("window.get_from_explorer").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, cy, 140, btn_h, parent,
        (HMENU)(INT_PTR)IDC_GEOM_GET_BTN, nullptr, nullptr);
    SendMessageW(geom_get_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    cy += btn_h + 5;

    // --- Content area: ListBox (left) + buttons (right) ---
    int action_btn_w = 85;
    int action_area_w = action_btn_w + 10;
    int listbox_w = w - action_area_w;
    int remaining_h = y + h - cy - btn_h - 5 - btn_h - 5 - btn_h - 5;

    listbox_ = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
        x, cy, listbox_w, remaining_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_LISTBOX, nullptr, nullptr);
    SendMessageW(listbox_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Action buttons
    int abx = x + listbox_w + 5;
    int aby = cy;

    move_up_btn_ = CreateWindowW(L"BUTTON", t("path.move_up").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, abx, aby, action_btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_MOVE_UP_BTN, nullptr, nullptr);
    SendMessageW(move_up_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    aby += btn_h + 3;

    move_down_btn_ = CreateWindowW(L"BUTTON", t("path.move_down").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, abx, aby, action_btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_MOVE_DOWN_BTN, nullptr, nullptr);
    SendMessageW(move_down_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    aby += btn_h + 3;

    add_path_btn_ = CreateWindowW(L"BUTTON", t("path.add").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, abx, aby, action_btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_ADD_BTN, nullptr, nullptr);
    SendMessageW(add_path_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    aby += btn_h + 3;

    remove_btn_ = CreateWindowW(L"BUTTON", t("path.remove").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, abx, aby, action_btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_REMOVE_BTN, nullptr, nullptr);
    SendMessageW(remove_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    aby += btn_h + 3;

    browse_btn_ = CreateWindowW(L"BUTTON", t("path.browse").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, abx, aby, action_btn_w, btn_h, parent,
        (HMENU)(INT_PTR)IDC_PATH_BROWSE_BTN, nullptr, nullptr);
    SendMessageW(browse_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    cy += remaining_h + 5;

    // --- Path entry ---
    path_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, cy, w, btn_h - 2, parent,
        (HMENU)(INT_PTR)IDC_PATH_ENTRY, nullptr, nullptr);
    SendMessageW(path_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    SendMessageW(path_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());
    cy += btn_h + 5;

    // --- Open as Tabs button ---
    open_btn_ = CreateWindowW(L"BUTTON", t("tab.open_as_tabs").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, cy, w, btn_h + 10, parent,
        (HMENU)(INT_PTR)IDC_OPEN_AS_TABS_BTN, nullptr, nullptr);
    SendMessageW(open_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Collect all controls for show/hide
    all_controls_ = {
        add_btn_, del_btn_, rename_btn_, copy_btn_, left_btn_, right_btn_,
        geom_x_label_, geom_x_entry_, geom_y_label_, geom_y_entry_,
        geom_w_label_, geom_w_entry_, geom_h_label_, geom_h_entry_, geom_get_btn_,
        listbox_, move_up_btn_, move_down_btn_, add_path_btn_, remove_btn_, browse_btn_,
        path_entry_, open_btn_
    };
    // TabView hwnd is managed separately

    load_tabs_from_config();
}

void TabGroupSection::resize(int x, int y, int w, int h) {
    x_ = x; y_ = y; w_ = w; h_ = h;

    int btn_w = 85, btn_h = 26, small_btn = 30, pad = 3;
    int cx = x, cy = y;

    // --- Tab management bar ---
    MoveWindow(add_btn_, cx, cy, btn_w, btn_h, TRUE); cx += btn_w + pad;
    MoveWindow(del_btn_, cx, cy, btn_w, btn_h, TRUE); cx += btn_w + pad;
    MoveWindow(rename_btn_, cx, cy, btn_w, btn_h, TRUE); cx += btn_w + pad;
    MoveWindow(copy_btn_, cx, cy, btn_w, btn_h, TRUE); cx += btn_w + 10;
    MoveWindow(left_btn_, cx, cy, small_btn, btn_h, TRUE); cx += small_btn + pad;
    MoveWindow(right_btn_, cx, cy, small_btn, btn_h, TRUE);
    cy += btn_h + 5;

    // --- Tab view ---
    int tab_view_h = TabView::VISIBLE_ROWS * (TabView::BTN_HEIGHT + TabView::BTN_PAD_Y * 2);
    tab_view_.resize(x, cy, w, tab_view_h);
    cy += tab_view_h + 5;

    // --- Geometry bar ---
    int geom_entry_w = 55, geom_label_w = 40;
    cx = x;
    MoveWindow(geom_x_label_, cx, cy, 20, btn_h, TRUE); cx += 20;
    MoveWindow(geom_x_entry_, cx, cy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_y_label_, cx, cy, 20, btn_h, TRUE); cx += 20;
    MoveWindow(geom_y_entry_, cx, cy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_w_label_, cx, cy, geom_label_w, btn_h, TRUE); cx += geom_label_w;
    MoveWindow(geom_w_entry_, cx, cy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_h_label_, cx, cy, geom_label_w, btn_h, TRUE); cx += geom_label_w;
    MoveWindow(geom_h_entry_, cx, cy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_get_btn_, cx, cy, 140, btn_h, TRUE);
    cy += btn_h + 5;

    // --- Content area ---
    int action_btn_w = 85;
    int action_area_w = action_btn_w + 10;
    int listbox_w = w - action_area_w;
    int remaining_h = y + h - cy - btn_h - 5 - (btn_h + 10) - 5 - btn_h - 5;
    if (remaining_h < 60) remaining_h = 60;

    MoveWindow(listbox_, x, cy, listbox_w, remaining_h, TRUE);

    int abx = x + listbox_w + 5;
    int aby = cy;
    MoveWindow(move_up_btn_, abx, aby, action_btn_w, btn_h, TRUE); aby += btn_h + 3;
    MoveWindow(move_down_btn_, abx, aby, action_btn_w, btn_h, TRUE); aby += btn_h + 3;
    MoveWindow(add_path_btn_, abx, aby, action_btn_w, btn_h, TRUE); aby += btn_h + 3;
    MoveWindow(remove_btn_, abx, aby, action_btn_w, btn_h, TRUE); aby += btn_h + 3;
    MoveWindow(browse_btn_, abx, aby, action_btn_w, btn_h, TRUE);

    cy += remaining_h + 5;
    MoveWindow(path_entry_, x, cy, w, btn_h - 2, TRUE);
    cy += btn_h + 5;
    MoveWindow(open_btn_, x, cy, w, btn_h + 10, TRUE);
}

bool TabGroupSection::handle_command(WPARAM wParam, LPARAM /*lParam*/) {
    int id = LOWORD(wParam);
    switch (id) {
    case IDC_TAB_ADD_BTN: on_add_tab(); return true;
    case IDC_TAB_DELETE_BTN: on_delete_tab(); return true;
    case IDC_TAB_RENAME_BTN: on_rename_tab(); return true;
    case IDC_TAB_COPY_BTN: on_copy_tab(); return true;
    case IDC_TAB_MOVE_LEFT_BTN: on_move_tab_left(); return true;
    case IDC_TAB_MOVE_RIGHT_BTN: on_move_tab_right(); return true;
    case IDC_PATH_ADD_BTN: on_add_path(); return true;
    case IDC_PATH_REMOVE_BTN: on_remove_path(); return true;
    case IDC_PATH_MOVE_UP_BTN: on_move_up(); return true;
    case IDC_PATH_MOVE_DOWN_BTN: on_move_down(); return true;
    case IDC_PATH_BROWSE_BTN: on_browse(); return true;
    case IDC_GEOM_GET_BTN: on_get_explorer_bounds(); return true;
    case IDC_OPEN_AS_TABS_BTN: on_open_as_tabs(); return true;
    case IDC_PATH_LISTBOX:
        if (HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = (int)SendMessageW(listbox_, LB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                int len = (int)SendMessageW(listbox_, LB_GETTEXTLEN, sel, 0);
                std::wstring text(len, L'\0');
                SendMessageW(listbox_, LB_GETTEXT, sel, (LPARAM)text.data());
                SetWindowTextW(path_entry_, text.c_str());
            }
        }
        return true;
    case IDC_PATH_ENTRY:
        if (HIWORD(wParam) == EN_CHANGE) return false;
        return false;
    }
    return false;
}

void TabGroupSection::load_tabs_from_config() {
    if (config_.data().tab_groups.empty()) {
        config_.add_tab_group(L"Tab 1");
        config_.save();
    }
    for (const auto& g : config_.data().tab_groups) {
        tab_view_.add_tab(g.name);
    }
    auto names = tab_view_.tab_names();
    LOG_INFO("tab_group", "load_tabs: %d tabs, setting current to '%s'",
             (int)names.size(), names.empty() ? "" : wide_to_utf8(names[0]).c_str());
    if (!names.empty()) {
        tab_view_.set_current_tab(names[0]);
        current_tab_ = names[0];
        LOG_INFO("tab_group", "current_tab_ set to '%s'", wide_to_utf8(current_tab_).c_str());
        refresh_listbox();
        load_geometry();
    }
}

void TabGroupSection::refresh_listbox() {
    SendMessageW(listbox_, LB_RESETCONTENT, 0, 0);
    if (current_tab_.empty()) return;
    auto* group = config_.get_tab_group(current_tab_);
    if (!group) return;
    for (const auto& p : group->paths) {
        SendMessageW(listbox_, LB_ADDSTRING, 0, (LPARAM)p.c_str());
    }
    // Set horizontal scroll extent
    HDC hdc = GetDC(listbox_);
    HFONT old = (HFONT)SelectObject(hdc, font_);
    int max_w = 0;
    for (const auto& p : group->paths) {
        SIZE sz;
        GetTextExtentPoint32W(hdc, p.c_str(), (int)p.size(), &sz);
        if (sz.cx > max_w) max_w = sz.cx;
    }
    SelectObject(hdc, old);
    ReleaseDC(listbox_, hdc);
    SendMessageW(listbox_, LB_SETHORIZONTALEXTENT, max_w + 20, 0);
}

void TabGroupSection::load_geometry() {
    SetWindowTextW(geom_x_entry_, L"");
    SetWindowTextW(geom_y_entry_, L"");
    SetWindowTextW(geom_w_entry_, L"");
    SetWindowTextW(geom_h_entry_, L"");
    if (current_tab_.empty()) return;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g) return;
    if (g->window_x) SetWindowTextW(geom_x_entry_, std::to_wstring(*g->window_x).c_str());
    if (g->window_y) SetWindowTextW(geom_y_entry_, std::to_wstring(*g->window_y).c_str());
    if (g->window_width) SetWindowTextW(geom_w_entry_, std::to_wstring(*g->window_width).c_str());
    if (g->window_height) SetWindowTextW(geom_h_entry_, std::to_wstring(*g->window_height).c_str());
}

void TabGroupSection::save_geometry() {
    if (current_tab_.empty()) return;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g) return;
    g->window_x = parse_int(geom_x_entry_);
    g->window_y = parse_int(geom_y_entry_);
    g->window_width = parse_int(geom_w_entry_);
    g->window_height = parse_int(geom_h_entry_);
    if (g->window_width && *g->window_width < MIN_WINDOW_WIDTH)
        g->window_width = MIN_WINDOW_WIDTH;
    if (g->window_height && *g->window_height < MIN_WINDOW_HEIGHT)
        g->window_height = MIN_WINDOW_HEIGHT;
    config_.save();
}

std::optional<int> TabGroupSection::parse_int(HWND edit) {
    wchar_t buf[64];
    GetWindowTextW(edit, buf, 64);
    std::wstring text(buf);
    if (text.empty()) return std::nullopt;
    try { return std::stoi(text); }
    catch (...) { return std::nullopt; }
}

void TabGroupSection::on_tab_changed(const std::wstring& name) {
    save_geometry();
    current_tab_ = name;
    refresh_listbox();
    load_geometry();
}

void TabGroupSection::on_add_tab() {
    auto result = show_input_dialog(parent_,
        t("tab.add_dialog_title"), t("tab.add_dialog_prompt"));
    if (!result || result->empty()) return;
    std::wstring name = *result;
    // Trim
    while (!name.empty() && iswspace(name.front())) name.erase(name.begin());
    while (!name.empty() && iswspace(name.back())) name.pop_back();
    if (name.empty()) return;

    if (config_.get_tab_group(name)) {
        MessageBoxW(parent_, t("tab.duplicate_msg", {{"name", name}}).c_str(),
                     t("tab.duplicate_title").c_str(), MB_ICONWARNING);
        return;
    }
    config_.add_tab_group(name);
    config_.save();
    tab_view_.add_tab(name);
    tab_view_.set_current_tab(name);
    current_tab_ = name;
    refresh_listbox();
    load_geometry();
}

void TabGroupSection::on_delete_tab() {
    auto name = tab_view_.get_current_tab();
    if (name.empty()) return;
    int result = MessageBoxW(parent_,
        t("tab.delete_confirm_msg", {{"name", name}}).c_str(),
        t("tab.delete_confirm_title").c_str(), MB_YESNO | MB_ICONQUESTION);
    if (result == IDYES) {
        config_.delete_tab_group(name);
        config_.save();
        tab_view_.delete_tab(name);
        current_tab_ = tab_view_.get_current_tab();
        refresh_listbox();
        load_geometry();
    }
}

void TabGroupSection::on_rename_tab() {
    auto old_name = tab_view_.get_current_tab();
    if (old_name.empty()) return;
    auto result = show_input_dialog(parent_,
        t("tab.rename_dialog_title"), t("tab.rename_dialog_prompt"), old_name);
    if (!result || result->empty() || *result == old_name) return;
    std::wstring new_name = *result;
    while (!new_name.empty() && iswspace(new_name.front())) new_name.erase(new_name.begin());
    while (!new_name.empty() && iswspace(new_name.back())) new_name.pop_back();
    if (new_name.empty() || new_name == old_name) return;

    if (config_.get_tab_group(new_name)) {
        MessageBoxW(parent_, t("tab.duplicate_msg", {{"name", new_name}}).c_str(),
                     t("tab.duplicate_title").c_str(), MB_ICONWARNING);
        return;
    }
    config_.rename_tab_group(old_name, new_name);
    config_.save();
    tab_view_.rename_tab(old_name, new_name);
    current_tab_ = new_name;
}

void TabGroupSection::on_copy_tab() {
    auto name = tab_view_.get_current_tab();
    if (name.empty()) return;
    auto* ng = config_.copy_tab_group(name);
    if (!ng) return;
    config_.save();
    tab_view_.add_tab(ng->name);
    tab_view_.set_current_tab(ng->name);
    current_tab_ = ng->name;
    refresh_listbox();
    load_geometry();
}

void TabGroupSection::on_move_tab_left() {
    auto name = tab_view_.get_current_tab();
    if (name.empty()) return;
    auto names = tab_view_.tab_names();
    auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end() || it == names.begin()) return;
    int idx = (int)(it - names.begin());
    config_.move_tab_group(idx, idx - 1);
    config_.save();
    tab_view_.move_tab(idx, idx - 1);
}

void TabGroupSection::on_move_tab_right() {
    auto name = tab_view_.get_current_tab();
    if (name.empty()) return;
    auto names = tab_view_.tab_names();
    auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end()) return;
    int idx = (int)(it - names.begin());
    if (idx >= (int)names.size() - 1) return;
    config_.move_tab_group(idx, idx + 1);
    config_.save();
    tab_view_.move_tab(idx, idx + 1);
}

void TabGroupSection::on_add_path() {
    wchar_t buf[2048];
    GetWindowTextW(path_entry_, buf, 2048);
    std::wstring raw(buf);
    while (!raw.empty() && iswspace(raw.front())) raw.erase(raw.begin());
    while (!raw.empty() && iswspace(raw.back())) raw.pop_back();
    std::wstring path = strip_quotes(raw);
    if (path.empty()) return;

    if (current_tab_.empty()) {
        MessageBoxW(parent_, t("tab.no_tab_msg").c_str(),
                     t("tab.no_tab_title").c_str(), MB_ICONINFORMATION);
        return;
    }
    std::wstring expanded = expand_user(path);
    if (!is_unc_path(expanded) && !is_directory(expanded)) {
        MessageBoxW(parent_, t("path.invalid_msg", {{"path", path}}).c_str(),
                     t("path.invalid_title").c_str(), MB_ICONWARNING);
        return;
    }
    config_.add_path_to_group(current_tab_, expanded);
    config_.save();
    refresh_listbox();
    SetWindowTextW(path_entry_, L"");
    LOG_INFO("tab_group", "Path added to '%s': %s",
             wide_to_utf8(current_tab_).c_str(), wide_to_utf8(expanded).c_str());
}

void TabGroupSection::on_remove_path() {
    int sel = (int)SendMessageW(listbox_, LB_GETCURSEL, 0, 0);
    if (sel < 0 || current_tab_.empty()) return;
    config_.remove_path_from_group(current_tab_, sel);
    config_.save();
    refresh_listbox();
}

void TabGroupSection::on_move_up() {
    int sel = (int)SendMessageW(listbox_, LB_GETCURSEL, 0, 0);
    if (sel <= 0 || current_tab_.empty()) return;
    config_.move_path_in_group(current_tab_, sel, sel - 1);
    config_.save();
    refresh_listbox();
    SendMessageW(listbox_, LB_SETCURSEL, sel - 1, 0);
}

void TabGroupSection::on_move_down() {
    int sel = (int)SendMessageW(listbox_, LB_GETCURSEL, 0, 0);
    if (sel < 0 || current_tab_.empty()) return;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g || sel >= (int)g->paths.size() - 1) return;
    config_.move_path_in_group(current_tab_, sel, sel + 1);
    config_.save();
    refresh_listbox();
    SendMessageW(listbox_, LB_SETCURSEL, sel + 1, 0);
}

void TabGroupSection::on_browse() {
    ComPtr<IFileOpenDialog> pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pDialog));
    if (FAILED(hr)) return;

    DWORD options;
    pDialog->GetOptions(&options);
    pDialog->SetOptions(options | FOS_PICKFOLDERS);

    hr = pDialog->Show(parent_);
    if (SUCCEEDED(hr)) {
        ComPtr<IShellItem> pItem;
        pDialog->GetResult(&pItem);
        if (pItem) {
            PWSTR path = nullptr;
            pItem->GetDisplayName(SIGDN_FILESYSPATH, &path);
            if (path) {
                std::wstring normalized = normalize_path(path);
                SetWindowTextW(path_entry_, normalized.c_str());
                CoTaskMemFree(path);
            }
        }
    }
}

void TabGroupSection::on_get_explorer_bounds() {
    auto rect = get_frontmost_explorer_rect();
    if (!rect) {
        MessageBoxW(parent_, t("window.no_explorer_msg").c_str(),
                     t("window.no_explorer_title").c_str(), MB_ICONINFORMATION);
        return;
    }
    SetWindowTextW(geom_x_entry_, std::to_wstring(rect->x).c_str());
    SetWindowTextW(geom_y_entry_, std::to_wstring(rect->y).c_str());
    SetWindowTextW(geom_w_entry_, std::to_wstring(rect->width).c_str());
    SetWindowTextW(geom_h_entry_, std::to_wstring(rect->height).c_str());
    save_geometry();
}

std::optional<WindowRect> TabGroupSection::get_window_rect() {
    save_geometry();
    if (current_tab_.empty()) return std::nullopt;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g) return std::nullopt;
    if (g->window_x && g->window_y && g->window_width && g->window_height) {
        return WindowRect{*g->window_x, *g->window_y, *g->window_width, *g->window_height};
    }
    return std::nullopt;
}

void TabGroupSection::on_open_as_tabs() {
    if (opening_) return;
    if (current_tab_.empty()) {
        MessageBoxW(parent_, t("tab.no_tab_msg").c_str(),
                     t("tab.no_tab_title").c_str(), MB_ICONINFORMATION);
        return;
    }
    auto* g = config_.get_tab_group(current_tab_);
    if (!g || g->paths.empty()) {
        MessageBoxW(parent_, t("tab.no_paths_msg").c_str(),
                     t("tab.no_paths_title").c_str(), MB_ICONINFORMATION);
        return;
    }
    opening_ = true;
    LOG_INFO("tab_group", "Opening as tabs: '%s', %d paths",
             wide_to_utf8(current_tab_).c_str(), (int)g->paths.size());
    on_open_tabs_(g->paths, get_window_rect());
}

void TabGroupSection::refresh_texts() {
    SetWindowTextW(add_btn_, t("tab.add").c_str());
    SetWindowTextW(del_btn_, t("tab.delete").c_str());
    SetWindowTextW(rename_btn_, t("tab.rename").c_str());
    SetWindowTextW(copy_btn_, t("tab.copy").c_str());
    SetWindowTextW(left_btn_, t("tab.move_left").c_str());
    SetWindowTextW(right_btn_, t("tab.move_right").c_str());
    SetWindowTextW(geom_x_label_, t("window.x").c_str());
    SetWindowTextW(geom_y_label_, t("window.y").c_str());
    SetWindowTextW(geom_w_label_, t("window.width").c_str());
    SetWindowTextW(geom_h_label_, t("window.height").c_str());
    SetWindowTextW(geom_get_btn_, t("window.get_from_explorer").c_str());
    SetWindowTextW(move_up_btn_, t("path.move_up").c_str());
    SetWindowTextW(move_down_btn_, t("path.move_down").c_str());
    SetWindowTextW(add_path_btn_, t("path.add").c_str());
    SetWindowTextW(remove_btn_, t("path.remove").c_str());
    SetWindowTextW(browse_btn_, t("path.browse").c_str());
    SendMessageW(path_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());
    SetWindowTextW(open_btn_, t("tab.open_as_tabs").c_str());
}

void TabGroupSection::show(bool visible) {
    int cmd = visible ? SW_SHOW : SW_HIDE;
    for (HWND h : all_controls_) {
        if (h) ShowWindow(h, cmd);
    }
    // TabView has its own hwnd
    if (IsWindow(tab_view_.hwnd())) {
        ShowWindow(tab_view_.hwnd(), cmd);
    }
}

void TabGroupSection::select_tab(const std::wstring& name) {
    if (name.empty()) return;
    tab_view_.set_current_tab(name);
    on_tab_changed(name);
}

} // namespace fto

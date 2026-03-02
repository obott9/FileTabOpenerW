#include "modern_tab_group_section.h"
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
#include <uxtheme.h>
#include <shellapi.h>

using Microsoft::WRL::ComPtr;

namespace fto {

ModernTabGroupSection::ModernTabGroupSection(ConfigManager& config, OpenTabsCallback on_open_tabs)
    : config_(config), on_open_tabs_(on_open_tabs) {}

ModernTabGroupSection::~ModernTabGroupSection() {
    if (font_) DeleteObject(font_);
}

void ModernTabGroupSection::create(HWND parent, int x, int y, int w, int h) {
    parent_ = parent;
    x_ = x; y_ = y; w_ = w; h_ = h;
    font_ = create_system_message_font();

    int btn_h = 26;

    // --- Sidebar ListView ---
    sidebar_ = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER |
        LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL |
        LVS_SHOWSELALWAYS | LVS_EDITLABELS,
        x, y, SIDEBAR_WIDTH, h - btn_h - 5 - btn_h - 5,
        parent, (HMENU)(INT_PTR)IDC_MODERN_SIDEBAR, nullptr, nullptr);
    SendMessageW(sidebar_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Add a single column that fills the width
    LVCOLUMNW col = {};
    col.mask = LVCF_WIDTH;
    col.cx = SIDEBAR_WIDTH - 4;
    ListView_InsertColumn(sidebar_, 0, &col);

    // Enable full-row select
    ListView_SetExtendedListViewStyle(sidebar_,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    // Dark mode theme for ListView
    if (dark_mode_) {
        SetWindowTheme(sidebar_, L"DarkMode_Explorer", nullptr);
    }

    // --- New group entry + button (below sidebar) ---
    int sidebar_bottom = y + h - btn_h - 5 - btn_h - 5;
    int add_btn_w = 30;

    new_group_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        x, sidebar_bottom + 5, SIDEBAR_WIDTH - add_btn_w - 3, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_NEW_GROUP_ENTRY, nullptr, nullptr);
    SendMessageW(new_group_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    SendMessageW(new_group_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("modern.new_group_placeholder").c_str());

    new_group_btn_ = CreateWindowW(L"BUTTON", t("modern.add").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x + SIDEBAR_WIDTH - add_btn_w, sidebar_bottom + 5, add_btn_w, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_NEW_GROUP_BTN, nullptr, nullptr);
    SendMessageW(new_group_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    // --- Detail panel controls ---
    int dx = x + SIDEBAR_WIDTH + GAP;
    int dw = w - SIDEBAR_WIDTH - GAP;
    int dy = y;

    // Tab name edit (header)
    tab_name_edit_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        dx, dy, dw, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_TAB_NAME, nullptr, nullptr);
    SendMessageW(tab_name_edit_, WM_SETFONT, (WPARAM)font_, TRUE);
    dy += btn_h + 5;

    // Geometry row
    int geom_entry_w = 55, geom_label_w = 40;
    int cx = dx;

    geom_x_label_ = CreateWindowW(L"STATIC", t("window.x").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, dy, 20, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_X_LABEL, nullptr, nullptr);
    SendMessageW(geom_x_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += 20;
    geom_x_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, dy, geom_entry_w, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_X_ENTRY, nullptr, nullptr);
    SendMessageW(geom_x_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_y_label_ = CreateWindowW(L"STATIC", t("window.y").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, dy, 20, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_Y_LABEL, nullptr, nullptr);
    SendMessageW(geom_y_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += 20;
    geom_y_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, dy, geom_entry_w, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_Y_ENTRY, nullptr, nullptr);
    SendMessageW(geom_y_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_w_label_ = CreateWindowW(L"STATIC", t("window.width").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, dy, geom_label_w, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_W_LABEL, nullptr, nullptr);
    SendMessageW(geom_w_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_label_w;
    geom_w_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, dy, geom_entry_w, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_W_ENTRY, nullptr, nullptr);
    SendMessageW(geom_w_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_h_label_ = CreateWindowW(L"STATIC", t("window.height").c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, cx, dy, geom_label_w, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_H_LABEL, nullptr, nullptr);
    SendMessageW(geom_h_label_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_label_w;
    geom_h_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER, cx, dy, geom_entry_w, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_H_ENTRY, nullptr, nullptr);
    SendMessageW(geom_h_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    cx += geom_entry_w + 8;

    geom_get_btn_ = CreateWindowW(L"BUTTON", t("window.get_from_explorer").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, cx, dy, 140, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_GEOM_GET_BTN, nullptr, nullptr);
    SendMessageW(geom_get_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    dy += btn_h + 5;

    // Path list (LISTBOX with drag support)
    int path_list_h = h - (dy - y) - btn_h - 5 - (btn_h + 10) - 5 - btn_h - 5;
    if (path_list_h < 60) path_list_h = 60;

    path_list_ = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY,
        dx, dy, dw, path_list_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_PATH_LIST, nullptr, nullptr);
    SendMessageW(path_list_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Make it a drag list for reordering
    MakeDragList(path_list_);
    drag_list_msg_ = RegisterWindowMessageW(DRAGLISTMSGSTRING);

    dy += path_list_h + 5;

    // Path entry + Add + Browse
    int browse_w = 30;
    int add_path_btn_w = 40;
    int entry_w = dw - add_path_btn_w - 3 - browse_w - 3;

    path_entry_ = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        dx, dy, entry_w, btn_h - 2,
        parent, (HMENU)(INT_PTR)IDC_MODERN_PATH_ENTRY, nullptr, nullptr);
    SendMessageW(path_entry_, WM_SETFONT, (WPARAM)font_, TRUE);
    SendMessageW(path_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());

    add_btn_ = CreateWindowW(L"BUTTON", t("path.add").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        dx + entry_w + 3, dy, add_path_btn_w, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_ADD_BTN, nullptr, nullptr);
    SendMessageW(add_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    browse_btn_ = CreateWindowW(L"BUTTON", L"...",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        dx + entry_w + 3 + add_path_btn_w + 3, dy, browse_w, btn_h,
        parent, (HMENU)(INT_PTR)IDC_MODERN_BROWSE_BTN, nullptr, nullptr);
    SendMessageW(browse_btn_, WM_SETFONT, (WPARAM)font_, TRUE);
    dy += btn_h + 5;

    // Open as Tabs button
    open_btn_ = CreateWindowW(L"BUTTON", t("tab.open_as_tabs").c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        dx, dy, dw, btn_h + 10,
        parent, (HMENU)(INT_PTR)IDC_MODERN_OPEN_BTN, nullptr, nullptr);
    SendMessageW(open_btn_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Placeholder label (shown when no group selected)
    placeholder_label_ = CreateWindowW(L"STATIC", t("modern.select_group").c_str(),
        WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
        dx, y, dw, h,
        parent, nullptr, nullptr, nullptr);
    SendMessageW(placeholder_label_, WM_SETFONT, (WPARAM)font_, TRUE);

    // Collect all controls
    all_controls_ = {
        sidebar_, new_group_entry_, new_group_btn_,
        tab_name_edit_,
        geom_x_label_, geom_x_entry_, geom_y_label_, geom_y_entry_,
        geom_w_label_, geom_w_entry_, geom_h_label_, geom_h_entry_, geom_get_btn_,
        path_list_, path_entry_, add_btn_, browse_btn_, open_btn_,
        placeholder_label_
    };

    load_groups_to_sidebar();

    // Select first group if available
    if (!config_.data().tab_groups.empty()) {
        ListView_SetItemState(sidebar_, 0, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    } else {
        refresh_detail();
    }
}

void ModernTabGroupSection::resize(int x, int y, int w, int h) {
    x_ = x; y_ = y; w_ = w; h_ = h;
    int btn_h = 26;

    // Sidebar
    int sidebar_h = h - btn_h - 5 - btn_h - 5;
    MoveWindow(sidebar_, x, y, SIDEBAR_WIDTH, sidebar_h, TRUE);

    // Update column width
    ListView_SetColumnWidth(sidebar_, 0, SIDEBAR_WIDTH - 4);

    // New group entry + button
    int add_btn_w = 30;
    int sidebar_bottom = y + sidebar_h;
    MoveWindow(new_group_entry_, x, sidebar_bottom + 5,
        SIDEBAR_WIDTH - add_btn_w - 3, btn_h - 2, TRUE);
    MoveWindow(new_group_btn_, x + SIDEBAR_WIDTH - add_btn_w, sidebar_bottom + 5,
        add_btn_w, btn_h, TRUE);

    // Detail panel
    int dx = x + SIDEBAR_WIDTH + GAP;
    int dw = w - SIDEBAR_WIDTH - GAP;
    int dy = y;

    MoveWindow(tab_name_edit_, dx, dy, dw, btn_h, TRUE);
    dy += btn_h + 5;

    // Geometry row
    int geom_entry_w = 55, geom_label_w = 40;
    int cx = dx;
    MoveWindow(geom_x_label_, cx, dy, 20, btn_h, TRUE); cx += 20;
    MoveWindow(geom_x_entry_, cx, dy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_y_label_, cx, dy, 20, btn_h, TRUE); cx += 20;
    MoveWindow(geom_y_entry_, cx, dy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_w_label_, cx, dy, geom_label_w, btn_h, TRUE); cx += geom_label_w;
    MoveWindow(geom_w_entry_, cx, dy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_h_label_, cx, dy, geom_label_w, btn_h, TRUE); cx += geom_label_w;
    MoveWindow(geom_h_entry_, cx, dy, geom_entry_w, btn_h - 2, TRUE); cx += geom_entry_w + 8;
    MoveWindow(geom_get_btn_, cx, dy, 140, btn_h, TRUE);
    dy += btn_h + 5;

    // Path list
    int path_list_h = h - (dy - y) - btn_h - 5 - (btn_h + 10) - 5 - btn_h - 5;
    if (path_list_h < 60) path_list_h = 60;
    MoveWindow(path_list_, dx, dy, dw, path_list_h, TRUE);
    dy += path_list_h + 5;

    // Path entry + Add + Browse
    int browse_w = 30;
    int add_path_btn_w = 40;
    int entry_w = dw - add_path_btn_w - 3 - browse_w - 3;
    MoveWindow(path_entry_, dx, dy, entry_w, btn_h - 2, TRUE);
    MoveWindow(add_btn_, dx + entry_w + 3, dy, add_path_btn_w, btn_h, TRUE);
    MoveWindow(browse_btn_, dx + entry_w + 3 + add_path_btn_w + 3, dy, browse_w, btn_h, TRUE);
    dy += btn_h + 5;

    // Open button
    MoveWindow(open_btn_, dx, dy, dw, btn_h + 10, TRUE);

    // Placeholder
    MoveWindow(placeholder_label_, dx, y, dw, h, TRUE);
}

bool ModernTabGroupSection::handle_command(WPARAM wParam, LPARAM /*lParam*/) {
    int id = LOWORD(wParam);

    // Handle drag list messages via parent forwarding
    // (drag list sends registered message, not WM_COMMAND)

    switch (id) {
    case IDC_MODERN_NEW_GROUP_BTN: on_add_group(); return true;
    case IDC_MODERN_ADD_BTN: on_add_path(); return true;
    case IDC_MODERN_BROWSE_BTN: on_browse(); return true;
    case IDC_MODERN_GEOM_GET_BTN: on_get_explorer_bounds(); return true;
    case IDC_MODERN_OPEN_BTN: on_open_as_tabs(); return true;
    case IDC_MODERN_TAB_NAME:
        if (HIWORD(wParam) == EN_CHANGE && !ignore_name_change_) {
            on_tab_name_changed();
        }
        return true;
    case IDC_MODERN_PATH_LIST:
        if (HIWORD(wParam) == LBN_SELCHANGE) {
            int sel = (int)SendMessageW(path_list_, LB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                int len = (int)SendMessageW(path_list_, LB_GETTEXTLEN, sel, 0);
                std::wstring text(len, L'\0');
                SendMessageW(path_list_, LB_GETTEXT, sel, (LPARAM)text.data());
                SetWindowTextW(path_entry_, text.c_str());
            }
        }
        return true;
    case IDC_MODERN_PATH_ENTRY:
        return false;

    // Context menu commands
    case IDM_MODERN_RENAME: on_rename_group(); return true;
    case IDM_MODERN_COPY: on_copy_group(); return true;
    case IDM_MODERN_DELETE: on_delete_group(); return true;
    case IDM_MODERN_MOVE_UP: on_move_group_up(); return true;
    case IDM_MODERN_MOVE_DOWN: on_move_group_down(); return true;
    case IDM_MODERN_PATH_REMOVE: {
        int sel = (int)SendMessageW(path_list_, LB_GETCURSEL, 0, 0);
        if (sel >= 0) on_remove_path_at(sel);
        return true;
    }
    case IDM_MODERN_PATH_OPEN_EXPLORER: {
        int sel = (int)SendMessageW(path_list_, LB_GETCURSEL, 0, 0);
        if (sel >= 0) {
            int len = (int)SendMessageW(path_list_, LB_GETTEXTLEN, sel, 0);
            std::wstring path(len, L'\0');
            SendMessageW(path_list_, LB_GETTEXT, sel, (LPARAM)path.data());
            ShellExecuteW(nullptr, L"explore", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        return true;
    }
    case IDM_MODERN_PATH_COPY_PATH: {
        int sel = (int)SendMessageW(path_list_, LB_GETCURSEL, 0, 0);
        if (sel >= 0) {
            int len = (int)SendMessageW(path_list_, LB_GETTEXTLEN, sel, 0);
            std::wstring path(len, L'\0');
            SendMessageW(path_list_, LB_GETTEXT, sel, (LPARAM)path.data());
            if (OpenClipboard(parent_)) {
                EmptyClipboard();
                size_t bytes = (path.size() + 1) * sizeof(wchar_t);
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (hMem) {
                    memcpy(GlobalLock(hMem), path.c_str(), bytes);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
            }
        }
        return true;
    }
    }
    return false;
}

bool ModernTabGroupSection::handle_notify(WPARAM wParam, LPARAM lParam) {
    auto* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    if (!nmhdr) return false;

    if (nmhdr->hwndFrom == sidebar_) {
        switch (nmhdr->code) {
        case LVN_ITEMCHANGED: {
            auto* nmlv = reinterpret_cast<NMLISTVIEW*>(lParam);
            if ((nmlv->uChanged & LVIF_STATE) &&
                (nmlv->uNewState & LVIS_SELECTED) &&
                !(nmlv->uOldState & LVIS_SELECTED)) {
                on_sidebar_selection_changed();
            }
            return true;
        }
        case NM_RCLICK:
            on_sidebar_right_click();
            return true;
        case LVN_BEGINLABELEDITW:
            on_sidebar_begin_label_edit(reinterpret_cast<NMLVDISPINFOW*>(lParam));
            return true;
        case LVN_ENDLABELEDITW:
            on_sidebar_end_label_edit(reinterpret_cast<NMLVDISPINFOW*>(lParam));
            return true;
        }
    }

    // Handle WM_CONTEXTMENU for path list
    if (nmhdr->hwndFrom == path_list_ && nmhdr->code == NM_RCLICK) {
        // This won't fire for LISTBOX - we handle it in WM_CONTEXTMENU
    }

    (void)wParam;
    return false;
}

void ModernTabGroupSection::save_geometry() {
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

void ModernTabGroupSection::refresh_texts() {
    SetWindowTextW(geom_x_label_, t("window.x").c_str());
    SetWindowTextW(geom_y_label_, t("window.y").c_str());
    SetWindowTextW(geom_w_label_, t("window.width").c_str());
    SetWindowTextW(geom_h_label_, t("window.height").c_str());
    SetWindowTextW(geom_get_btn_, t("window.get_from_explorer").c_str());
    SetWindowTextW(add_btn_, t("path.add").c_str());
    SendMessageW(path_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("path.placeholder").c_str());
    SetWindowTextW(open_btn_, t("tab.open_as_tabs").c_str());
    SetWindowTextW(new_group_btn_, t("modern.add").c_str());
    SendMessageW(new_group_entry_, EM_SETCUEBANNER, FALSE,
        (LPARAM)t("modern.new_group_placeholder").c_str());
    SetWindowTextW(placeholder_label_, t("modern.select_group").c_str());
}

void ModernTabGroupSection::set_dark_mode(bool dark) {
    dark_mode_ = dark;
    if (sidebar_) {
        SetWindowTheme(sidebar_, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        InvalidateRect(sidebar_, nullptr, TRUE);
    }
}

void ModernTabGroupSection::show(bool visible) {
    int cmd = visible ? SW_SHOW : SW_HIDE;
    for (HWND h : all_controls_) {
        if (h) ShowWindow(h, cmd);
    }
    if (visible) {
        refresh_detail(); // Update detail panel visibility
    }
}

void ModernTabGroupSection::select_tab(const std::wstring& name) {
    if (name.empty()) {
        // Select first group as fallback
        if (!config_.data().tab_groups.empty()) {
            select_tab(config_.data().tab_groups[0].name);
        }
        return;
    }
    int count = ListView_GetItemCount(sidebar_);
    for (int i = 0; i < count; i++) {
        wchar_t buf[256];
        ListView_GetItemText(sidebar_, i, 0, buf, 256);
        if (std::wstring(buf) == name) {
            ListView_SetItemState(sidebar_, i, LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(sidebar_, i, FALSE);
            // Explicitly update state — LVN_ITEMCHANGED may not fire
            // if the item was already selected (e.g. initial create while hidden)
            if (current_tab_ != name) {
                save_geometry();
                current_tab_ = name;
                LOG_INFO("modern", "Selected group (explicit): %s",
                         wide_to_utf8(current_tab_).c_str());
                refresh_detail();
            }
            break;
        }
    }
}

// --- Sidebar operations ---

void ModernTabGroupSection::load_groups_to_sidebar() {
    ListView_DeleteAllItems(sidebar_);
    for (int i = 0; i < (int)config_.data().tab_groups.size(); i++) {
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = (LPWSTR)config_.data().tab_groups[i].name.c_str();
        ListView_InsertItem(sidebar_, &item);
    }
}

void ModernTabGroupSection::on_sidebar_selection_changed() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)config_.data().tab_groups.size()) {
        save_geometry();
        current_tab_.clear();
        refresh_detail();
        return;
    }

    save_geometry();
    current_tab_ = config_.data().tab_groups[sel].name;
    LOG_INFO("modern", "Selected group: %s", wide_to_utf8(current_tab_).c_str());
    refresh_detail();
}

void ModernTabGroupSection::on_sidebar_right_click() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel < 0) return;

    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_MODERN_RENAME, t("tab.rename").c_str());
    AppendMenuW(menu, MF_STRING, IDM_MODERN_COPY, t("tab.copy").c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_MODERN_MOVE_UP, t("tab.move_left").c_str());
    AppendMenuW(menu, MF_STRING, IDM_MODERN_MOVE_DOWN, t("tab.move_right").c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_MODERN_DELETE, t("tab.delete").c_str());

    // Disable move up/down at boundaries
    if (sel == 0)
        EnableMenuItem(menu, IDM_MODERN_MOVE_UP, MF_GRAYED);
    if (sel >= (int)config_.data().tab_groups.size() - 1)
        EnableMenuItem(menu, IDM_MODERN_MOVE_DOWN, MF_GRAYED);

    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, parent_, nullptr);
    DestroyMenu(menu);
}

void ModernTabGroupSection::on_sidebar_begin_label_edit(NMLVDISPINFOW* /*pdi*/) {
    // Allow editing - don't cancel
}

void ModernTabGroupSection::on_sidebar_end_label_edit(NMLVDISPINFOW* pdi) {
    if (!pdi->item.pszText) return; // Editing cancelled

    std::wstring new_name(pdi->item.pszText);
    // Trim
    while (!new_name.empty() && iswspace(new_name.front())) new_name.erase(new_name.begin());
    while (!new_name.empty() && iswspace(new_name.back())) new_name.pop_back();

    if (new_name.empty()) return;
    if (new_name == current_tab_) return;

    if (config_.get_tab_group(new_name)) {
        MessageBoxW(parent_, t("tab.duplicate_msg", {{"name", new_name}}).c_str(),
                     t("tab.duplicate_title").c_str(), MB_ICONWARNING);
        return;
    }

    LOG_INFO("modern", "Tab group renamed: %s -> %s",
             wide_to_utf8(current_tab_).c_str(), wide_to_utf8(new_name).c_str());
    config_.rename_tab_group(current_tab_, new_name);
    config_.save();
    current_tab_ = new_name;

    // Accept the edit by setting result
    SetWindowLongPtrW(parent_, DWLP_MSGRESULT, TRUE);

    // Update the tab name edit in detail panel
    ignore_name_change_ = true;
    SetWindowTextW(tab_name_edit_, new_name.c_str());
    ignore_name_change_ = false;
}

// --- Tab group operations ---

void ModernTabGroupSection::on_add_group() {
    wchar_t buf[256];
    GetWindowTextW(new_group_entry_, buf, 256);
    std::wstring name(buf);
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
    LOG_INFO("modern", "Tab group added: %s", wide_to_utf8(name).c_str());

    // Add to sidebar
    LVITEMW item = {};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(sidebar_);
    item.pszText = (LPWSTR)name.c_str();
    ListView_InsertItem(sidebar_, &item);

    // Select the new group
    ListView_SetItemState(sidebar_, item.iItem, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(sidebar_, item.iItem, FALSE);

    SetWindowTextW(new_group_entry_, L"");
}

void ModernTabGroupSection::on_delete_group() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel < 0 || current_tab_.empty()) return;

    int result = MessageBoxW(parent_,
        t("tab.delete_confirm_msg", {{"name", current_tab_}}).c_str(),
        t("tab.delete_confirm_title").c_str(), MB_YESNO | MB_ICONQUESTION);
    if (result != IDYES) return;

    LOG_INFO("modern", "Tab group deleted: %s", wide_to_utf8(current_tab_).c_str());
    config_.delete_tab_group(current_tab_);
    config_.save();

    ListView_DeleteItem(sidebar_, sel);

    // Select next/previous item
    int count = ListView_GetItemCount(sidebar_);
    if (count > 0) {
        int new_sel = (sel < count) ? sel : count - 1;
        ListView_SetItemState(sidebar_, new_sel, LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    } else {
        current_tab_.clear();
        refresh_detail();
    }
}

void ModernTabGroupSection::on_rename_group() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel < 0) return;
    // Start inline editing
    ListView_EditLabel(sidebar_, sel);
}

void ModernTabGroupSection::on_copy_group() {
    if (current_tab_.empty()) return;
    auto* ng = config_.copy_tab_group(current_tab_);
    if (!ng) return;
    config_.save();
    LOG_INFO("modern", "Tab group copied: %s -> %s",
             wide_to_utf8(current_tab_).c_str(), wide_to_utf8(ng->name).c_str());

    load_groups_to_sidebar();

    // Select the new copy
    select_tab(ng->name);
}

void ModernTabGroupSection::on_move_group_up() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel <= 0) return;
    config_.move_tab_group(sel, sel - 1);
    config_.save();
    load_groups_to_sidebar();
    ListView_SetItemState(sidebar_, sel - 1, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED);
}

void ModernTabGroupSection::on_move_group_down() {
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= (int)config_.data().tab_groups.size() - 1) return;
    config_.move_tab_group(sel, sel + 1);
    config_.save();
    load_groups_to_sidebar();
    ListView_SetItemState(sidebar_, sel + 1, LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED);
}

// --- Detail panel ---

void ModernTabGroupSection::refresh_detail() {
    bool has_selection = !current_tab_.empty();

    // Show/hide detail controls vs placeholder
    HWND detail_controls[] = {
        tab_name_edit_,
        geom_x_label_, geom_x_entry_, geom_y_label_, geom_y_entry_,
        geom_w_label_, geom_w_entry_, geom_h_label_, geom_h_entry_, geom_get_btn_,
        path_list_, path_entry_, add_btn_, browse_btn_, open_btn_
    };

    int detail_cmd = has_selection ? SW_SHOW : SW_HIDE;
    for (HWND h : detail_controls) {
        if (h) ShowWindow(h, detail_cmd);
    }
    ShowWindow(placeholder_label_, has_selection ? SW_HIDE : SW_SHOW);

    if (has_selection) {
        ignore_name_change_ = true;
        SetWindowTextW(tab_name_edit_, current_tab_.c_str());
        ignore_name_change_ = false;
        refresh_path_list();
        load_geometry();
    }
}

void ModernTabGroupSection::refresh_path_list() {
    SendMessageW(path_list_, LB_RESETCONTENT, 0, 0);
    if (current_tab_.empty()) return;
    auto* group = config_.get_tab_group(current_tab_);
    if (!group) return;
    for (const auto& p : group->paths) {
        SendMessageW(path_list_, LB_ADDSTRING, 0, (LPARAM)p.c_str());
    }
    // Set horizontal scroll extent
    HDC hdc = GetDC(path_list_);
    HFONT old = (HFONT)SelectObject(hdc, font_);
    int max_w = 0;
    for (const auto& p : group->paths) {
        SIZE sz;
        GetTextExtentPoint32W(hdc, p.c_str(), (int)p.size(), &sz);
        if (sz.cx > max_w) max_w = sz.cx;
    }
    SelectObject(hdc, old);
    ReleaseDC(path_list_, hdc);
    SendMessageW(path_list_, LB_SETHORIZONTALEXTENT, max_w + 20, 0);
}

void ModernTabGroupSection::load_geometry() {
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

void ModernTabGroupSection::on_tab_name_changed() {
    if (current_tab_.empty()) return;

    wchar_t buf[256];
    GetWindowTextW(tab_name_edit_, buf, 256);
    std::wstring new_name(buf);
    while (!new_name.empty() && iswspace(new_name.front())) new_name.erase(new_name.begin());
    while (!new_name.empty() && iswspace(new_name.back())) new_name.pop_back();

    if (new_name.empty() || new_name == current_tab_) return;

    // Check for duplicates
    if (config_.get_tab_group(new_name)) return; // Silently ignore

    config_.rename_tab_group(current_tab_, new_name);
    config_.save();

    // Update sidebar
    int sel = ListView_GetNextItem(sidebar_, -1, LVNI_SELECTED);
    if (sel >= 0) {
        ListView_SetItemText(sidebar_, sel, 0, (LPWSTR)new_name.c_str());
    }

    current_tab_ = new_name;
    LOG_INFO("modern", "Tab name changed inline: %s", wide_to_utf8(new_name).c_str());
}

void ModernTabGroupSection::on_add_path() {
    wchar_t buf[2048];
    GetWindowTextW(path_entry_, buf, 2048);
    std::wstring raw(buf);
    while (!raw.empty() && iswspace(raw.front())) raw.erase(raw.begin());
    while (!raw.empty() && iswspace(raw.back())) raw.pop_back();
    std::wstring path = strip_quotes(raw);
    if (path.empty()) return;

    if (current_tab_.empty()) return;
    std::wstring expanded = expand_user(path);
    if (!is_unc_path(expanded) && !is_directory(expanded)) {
        MessageBoxW(parent_, t("path.invalid_msg", {{"path", path}}).c_str(),
                     t("path.invalid_title").c_str(), MB_ICONWARNING);
        return;
    }
    config_.add_path_to_group(current_tab_, expanded);
    config_.save();
    refresh_path_list();
    SetWindowTextW(path_entry_, L"");
    LOG_INFO("modern", "Path added to '%s': %s",
             wide_to_utf8(current_tab_).c_str(), wide_to_utf8(expanded).c_str());
}

void ModernTabGroupSection::on_remove_path_at(int index) {
    if (current_tab_.empty()) return;
    config_.remove_path_from_group(current_tab_, index);
    config_.save();
    refresh_path_list();
    LOG_INFO("modern", "Path removed at index %d", index);
}

void ModernTabGroupSection::on_browse() {
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

void ModernTabGroupSection::on_get_explorer_bounds() {
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

void ModernTabGroupSection::on_open_as_tabs() {
    if (opening_) return;
    if (current_tab_.empty()) return;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g || g->paths.empty()) {
        MessageBoxW(parent_, t("tab.no_paths_msg").c_str(),
                     t("tab.no_paths_title").c_str(), MB_ICONINFORMATION);
        return;
    }
    opening_ = true;
    LOG_INFO("modern", "Opening as tabs: '%s', %d paths",
             wide_to_utf8(current_tab_).c_str(), (int)g->paths.size());
    on_open_tabs_(g->paths, get_window_rect());
}

void ModernTabGroupSection::on_path_right_click() {
    int sel = (int)SendMessageW(path_list_, LB_GETCURSEL, 0, 0);
    if (sel < 0) return;

    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_MODERN_PATH_REMOVE, t("path.remove").c_str());
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, parent_, nullptr);
    DestroyMenu(menu);
}

std::optional<WindowRect> ModernTabGroupSection::get_window_rect() {
    save_geometry();
    if (current_tab_.empty()) return std::nullopt;
    auto* g = config_.get_tab_group(current_tab_);
    if (!g) return std::nullopt;
    if (g->window_x && g->window_y && g->window_width && g->window_height) {
        return WindowRect{*g->window_x, *g->window_y, *g->window_width, *g->window_height};
    }
    return std::nullopt;
}

std::optional<int> ModernTabGroupSection::parse_int(HWND edit) {
    wchar_t buf[64];
    GetWindowTextW(edit, buf, 64);
    std::wstring text(buf);
    if (text.empty()) return std::nullopt;
    try { return std::stoi(text); }
    catch (...) { return std::nullopt; }
}

} // namespace fto

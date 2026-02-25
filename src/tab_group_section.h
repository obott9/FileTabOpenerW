#pragma once
#include "config.h"
#include "tab_view.h"
#include <functional>
#include <optional>
#include <windows.h>

namespace fto {

struct WindowRect;

class TabGroupSection {
public:
    using OpenTabsCallback = std::function<void(
        const std::vector<std::wstring>& paths,
        const std::optional<WindowRect>& rect)>;

    TabGroupSection(ConfigManager& config, OpenTabsCallback on_open_tabs);
    ~TabGroupSection() { if (font_) DeleteObject(font_); }

    void create(HWND parent, int x, int y, int w, int h);
    void resize(int x, int y, int w, int h);
    bool handle_command(WPARAM wParam, LPARAM lParam);

    std::wstring current_tab_name() const { return current_tab_; }
    TabView& tab_view() { return tab_view_; }
    bool is_opening() const { return opening_; }
    void set_opening(bool v) { opening_ = v; }
    void save_geometry();
    void refresh_texts();

private:
    void load_tabs_from_config();
    void refresh_listbox();
    void load_geometry();

    void on_add_tab();
    void on_delete_tab();
    void on_rename_tab();
    void on_copy_tab();
    void on_move_tab_left();
    void on_move_tab_right();
    void on_add_path();
    void on_remove_path();
    void on_move_up();
    void on_move_down();
    void on_browse();
    void on_get_explorer_bounds();
    void on_open_as_tabs();
    void on_tab_changed(const std::wstring& name);

    std::optional<WindowRect> get_window_rect();
    static std::optional<int> parse_int(HWND edit);

    ConfigManager& config_;
    OpenTabsCallback on_open_tabs_;
    std::wstring current_tab_;
    bool opening_ = false;

    HWND parent_ = nullptr;
    HFONT font_ = nullptr;
    int x_ = 0, y_ = 0, w_ = 0, h_ = 0;

    // Tab management bar
    HWND add_btn_ = nullptr, del_btn_ = nullptr, rename_btn_ = nullptr;
    HWND copy_btn_ = nullptr, left_btn_ = nullptr, right_btn_ = nullptr;

    // Tab view
    TabView tab_view_;

    // Geometry inputs
    HWND geom_x_label_ = nullptr, geom_x_entry_ = nullptr;
    HWND geom_y_label_ = nullptr, geom_y_entry_ = nullptr;
    HWND geom_w_label_ = nullptr, geom_w_entry_ = nullptr;
    HWND geom_h_label_ = nullptr, geom_h_entry_ = nullptr;
    HWND geom_get_btn_ = nullptr;

    // Path list
    HWND listbox_ = nullptr;
    HWND move_up_btn_ = nullptr, move_down_btn_ = nullptr;
    HWND add_path_btn_ = nullptr, remove_btn_ = nullptr, browse_btn_ = nullptr;
    HWND path_entry_ = nullptr;

    // Open as Tabs
    HWND open_btn_ = nullptr;

    static constexpr int MIN_WINDOW_WIDTH = 528;
    static constexpr int MIN_WINDOW_HEIGHT = 308;
};

} // namespace fto

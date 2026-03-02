#pragma once
#include "tab_group_section.h"
#include <commctrl.h>

namespace fto {

// Modern layout: sidebar ListView + detail panel
class ModernTabGroupSection : public ITabGroupUI {
public:
    ModernTabGroupSection(ConfigManager& config, OpenTabsCallback on_open_tabs);
    ~ModernTabGroupSection() override;

    void create(HWND parent, int x, int y, int w, int h) override;
    void resize(int x, int y, int w, int h) override;
    bool handle_command(WPARAM wParam, LPARAM lParam) override;
    bool handle_notify(WPARAM wParam, LPARAM lParam) override;
    void save_geometry() override;
    void refresh_texts() override;
    std::wstring current_tab_name() const override { return current_tab_; }
    bool is_opening() const override { return opening_; }
    void set_opening(bool v) override { opening_ = v; }
    void set_dark_mode(bool dark) override;
    void show(bool visible) override;
    void select_tab(const std::wstring& name) override;

    bool is_created() const { return parent_ != nullptr; }

private:
    // Sidebar operations
    void load_groups_to_sidebar();
    void on_sidebar_selection_changed();
    void on_sidebar_right_click();
    void on_sidebar_end_label_edit(NMLVDISPINFOW* pdi);
    void on_sidebar_begin_label_edit(NMLVDISPINFOW* pdi);

    // Tab group operations
    void on_add_group();
    void on_delete_group();
    void on_rename_group();
    void on_copy_group();
    void on_move_group_up();
    void on_move_group_down();

    // Detail panel operations
    void refresh_detail();
    void refresh_path_list();
    void load_geometry();
    void on_tab_name_changed();
    void on_add_path();
    void on_remove_path_at(int index);
    void on_browse();
    void on_get_explorer_bounds();
    void on_open_as_tabs();

    // Path list context menu
    void on_path_right_click();

    std::optional<WindowRect> get_window_rect();
    static std::optional<int> parse_int(HWND edit);

    ConfigManager& config_;
    OpenTabsCallback on_open_tabs_;
    std::wstring current_tab_;
    bool opening_ = false;
    bool dark_mode_ = false;
    bool ignore_name_change_ = false;

    HWND parent_ = nullptr;
    HFONT font_ = nullptr;
    int x_ = 0, y_ = 0, w_ = 0, h_ = 0;

    static constexpr int SIDEBAR_WIDTH = 200;
    static constexpr int GAP = 8;

    // Sidebar
    HWND sidebar_ = nullptr;
    HWND new_group_entry_ = nullptr;
    HWND new_group_btn_ = nullptr;

    // Detail panel
    HWND tab_name_edit_ = nullptr;
    HWND geom_x_label_ = nullptr, geom_x_entry_ = nullptr;
    HWND geom_y_label_ = nullptr, geom_y_entry_ = nullptr;
    HWND geom_w_label_ = nullptr, geom_w_entry_ = nullptr;
    HWND geom_h_label_ = nullptr, geom_h_entry_ = nullptr;
    HWND geom_get_btn_ = nullptr;
    HWND path_list_ = nullptr;
    HWND path_entry_ = nullptr;
    HWND add_btn_ = nullptr;
    HWND browse_btn_ = nullptr;
    HWND open_btn_ = nullptr;

    // Placeholder label
    HWND placeholder_label_ = nullptr;

    // All controls for show/hide
    std::vector<HWND> all_controls_;

    // Drag list message
    UINT drag_list_msg_ = 0;

    static constexpr int MIN_WINDOW_WIDTH = 528;
    static constexpr int MIN_WINDOW_HEIGHT = 308;
};

} // namespace fto

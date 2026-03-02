#pragma once
#include "config.h"
#include "tab_view.h"
#include <functional>
#include <optional>
#include <windows.h>

namespace fto {

struct WindowRect;

// Abstract interface for tab group UI (Compact / Sidebar)
using OpenTabsCallback = std::function<void(
    const std::vector<std::wstring>& paths,
    const std::optional<WindowRect>& rect)>;

class ITabGroupUI {
public:
    virtual ~ITabGroupUI() = default;
    virtual void create(HWND parent, int x, int y, int w, int h) = 0;
    virtual void resize(int x, int y, int w, int h) = 0;
    virtual bool handle_command(WPARAM wParam, LPARAM lParam) = 0;
    virtual bool handle_notify(WPARAM wParam, LPARAM lParam) { (void)wParam; (void)lParam; return false; }
    virtual void save_geometry() = 0;
    virtual void refresh_texts() = 0;
    virtual std::wstring current_tab_name() const = 0;
    virtual bool is_opening() const = 0;
    virtual void set_opening(bool v) = 0;
    virtual void set_dark_mode(bool dark) = 0;
    virtual void show(bool visible) = 0;
    virtual void select_tab(const std::wstring& name) = 0;
};

// Compact layout: button bar + FlowLayout tab buttons + path listbox
class TabGroupSection : public ITabGroupUI {
public:
    TabGroupSection(ConfigManager& config, OpenTabsCallback on_open_tabs);
    ~TabGroupSection() override { if (font_) DeleteObject(font_); }

    void create(HWND parent, int x, int y, int w, int h) override;
    void resize(int x, int y, int w, int h) override;
    bool handle_command(WPARAM wParam, LPARAM lParam) override;
    void save_geometry() override;
    void refresh_texts() override;
    std::wstring current_tab_name() const override { return current_tab_; }
    TabView& tab_view() { return tab_view_; }
    bool is_opening() const override { return opening_; }
    void set_opening(bool v) override { opening_ = v; }
    void set_dark_mode(bool dark) override { tab_view_.set_dark_mode(dark); }
    void show(bool visible) override;
    void select_tab(const std::wstring& name) override;

    bool is_created() const { return parent_ != nullptr; }

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

    // All controls for show/hide
    std::vector<HWND> all_controls_;

    static constexpr int MIN_WINDOW_WIDTH = 528;
    static constexpr int MIN_WINDOW_HEIGHT = 308;
};

} // namespace fto

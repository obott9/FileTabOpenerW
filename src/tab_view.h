#pragma once
#include <string>
#include <vector>
#include <functional>
#include <windows.h>

namespace fto {

// Custom tab button bar with auto-wrapping rows and vertical scrollbar.
class TabView {
public:
    using TabChangedCallback = std::function<void(const std::wstring& name)>;

    TabView() = default;
    ~TabView() { if (font_) DeleteObject(font_); }

    void create(HWND parent, int x, int y, int w, int h, TabChangedCallback cb);
    void resize(int x, int y, int w, int h);

    void add_tab(const std::wstring& name);
    void delete_tab(const std::wstring& name);
    void rename_tab(const std::wstring& old_name, const std::wstring& new_name);
    void set_current_tab(const std::wstring& name);
    std::wstring get_current_tab() const { return current_; }
    void move_tab(int old_index, int new_index);
    std::vector<std::wstring> tab_names() const { return names_; }

    HWND hwnd() const { return hwnd_; }

    static void register_class();

    static constexpr int BTN_HEIGHT = 26;
    static constexpr int BTN_PAD_X = 2;
    static constexpr int BTN_PAD_Y = 1;
    static constexpr int VISIBLE_ROWS = 3;

    void set_dark_mode(bool dark) { dark_mode_ = dark; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void layout();
    void update_selection();
    void on_button_click(int btn_id);
    void draw_tab_button(const DRAWITEMSTRUCT* dis);

    HWND hwnd_ = nullptr;
    HWND parent_ = nullptr;
    std::vector<std::wstring> names_;
    std::vector<HWND> buttons_;
    std::wstring current_;
    TabChangedCallback on_tab_changed_;
    HFONT font_ = nullptr;
    int total_height_ = 0;
    int scroll_pos_ = 0;
    bool in_layout_ = false;  // Prevent reentrancy from SetScrollInfo -> WM_SIZE
    bool dark_mode_ = false;

    static constexpr int BASE_BTN_ID = 5000;
};

} // namespace fto

#pragma once
#include "config.h"
#include <functional>
#include <windows.h>

namespace fto {

class HistorySection {
public:
    using OpenFolderCallback = std::function<void(const std::wstring& path)>;

    HistorySection(ConfigManager& config, OpenFolderCallback on_open);

    void create(HWND parent, int x, int y, int w, int h);
    void resize(int x, int y, int w, int h);
    bool handle_command(WPARAM wParam, LPARAM lParam);
    bool is_created() const { return entry_ != nullptr && IsWindow(entry_); }

private:
    void on_open();
    void on_pin();
    void on_clear();
    void toggle_dropdown();
    void show_dropdown();
    void close_dropdown();
    std::wstring get_entry_text();
    void set_entry_text(const std::wstring& text);

    ConfigManager& config_;
    OpenFolderCallback on_open_folder_;
    HWND parent_ = nullptr;
    HWND label_ = nullptr;
    HWND entry_ = nullptr;
    HWND dropdown_btn_ = nullptr;
    HWND open_btn_ = nullptr;
    HWND pin_btn_ = nullptr;
    HWND clear_btn_ = nullptr;
    HWND dropdown_list_ = nullptr;  // popup listbox
    HWND dropdown_popup_ = nullptr; // popup window
    HFONT font_ = nullptr;
    int x_ = 0, y_ = 0, w_ = 0, h_ = 0;
};

} // namespace fto

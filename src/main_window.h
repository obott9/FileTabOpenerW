#pragma once
#include "config.h"
#include "history_section.h"
#include "tab_group_section.h"
#include <windows.h>
#include <memory>

namespace fto {

class MainWindow {
public:
    MainWindow(ConfigManager& config);

    void create();
    void run();
    HWND hwnd() const { return hwnd_; }

    static void register_class();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void on_create();
    void on_size(int w, int h);
    void on_close();
    void on_command(WPARAM wParam, LPARAM lParam);
    void on_tab_open_complete();
    void on_tab_open_error(const std::wstring& path, const std::wstring& error);

    void open_single_folder(const std::wstring& path);
    void open_folders_as_tabs(const std::vector<std::wstring>& paths,
                              const std::optional<WindowRect>& rect);
    void show_toast();
    void hide_toast();

    ConfigManager& config_;
    HWND hwnd_ = nullptr;
    HFONT font_ = nullptr;

    // Settings bar
    HWND timeout_label_ = nullptr;
    HWND timeout_combo_ = nullptr;
    HWND timeout_unit_ = nullptr;
    HWND lang_label_ = nullptr;
    HWND lang_combo_ = nullptr;

    // Separator
    HWND separator_ = nullptr;

    // Sections
    std::unique_ptr<HistorySection> history_;
    std::unique_ptr<TabGroupSection> tab_group_;

    // Toast overlay
    HWND toast_hwnd_ = nullptr;

    int client_w_ = 0, client_h_ = 0;
};

} // namespace fto

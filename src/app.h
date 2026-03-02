#pragma once
#include "config.h"
#include "main_window.h"
#include <memory>
#include <windows.h>

namespace fto {

// Dark mode detection
bool is_dark_mode();
void apply_dark_title_bar(HWND hwnd);

class App {
public:
    App();
    ~App();

    bool init();
    int run();

private:
    ConfigManager config_;
    std::unique_ptr<MainWindow> window_;
    HANDLE mutex_ = nullptr;
};

} // namespace fto

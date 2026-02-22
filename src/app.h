#pragma once
#include "config.h"
#include "main_window.h"
#include <memory>

namespace fto {

class App {
public:
    App();
    ~App();

    bool init();
    int run();

private:
    ConfigManager config_;
    std::unique_ptr<MainWindow> window_;
};

} // namespace fto

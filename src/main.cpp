#include "app.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
                    LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    fto::App app;
    if (!app.init()) return 1;
    return app.run();
}

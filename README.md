# FileTabOpenerW

[日本語](README_ja.md) | [한국어](README_ko.md) | [繁體中文](README_zh_TW.md) | [简体中文](README_zh_CN.md)

A native C++ Win32 application for managing and opening folders as Explorer tabs on Windows 11+.

This is the Windows-native port of [file_tab_opener](https://github.com/obott9/file_tab_opener) (Python/Tk), built with pure Win32 API for minimal dependencies and fast startup.

## Features

- **Tab Group Management** - Create, rename, copy, delete, and reorder tab groups
- **One-Click Open** - Open all folders in a tab group as Explorer tabs in a single window
- **Folder History** - Recently opened folders with pin support
- **Window Geometry** - Save and restore Explorer window position/size per tab group
- **Multi-Monitor** - Supports negative coordinates for multi-monitor setups
- **Dark Mode** - Follows Windows dark/light theme automatically
- **Internationalization** - English, Japanese, Korean, Traditional/Simplified Chinese
- **Portable Config** - JSON configuration file in `%APPDATA%\FileTabOpenerW`

## Requirements

- Windows 11 or later (Windows 10 may work but Explorer tab support requires Win11 22H2+)
- MSVC build tools (Visual Studio 2019+ or Build Tools for Visual Studio)
- CMake 3.20+

## Build

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

The executable will be at `build/Release/FileTabOpenerW.exe`.

## Usage

1. Launch `FileTabOpenerW.exe`
2. Create a tab group with **+ Add Tab**
3. Add folder paths using the path entry field or **Browse...**
4. Optionally set Explorer window geometry with **Get from Explorer**
5. Click **Open as Tabs** to open all folders as tabs in one Explorer window

### How It Works

The application uses a multi-strategy approach to open Explorer tabs:

1. **UI Automation (UIA)** - Primary method. Uses the Windows UI Automation API to find the Explorer's "New Tab" button and address bar, then programmatically creates tabs and navigates to each path.
2. **SendInput** - Fallback. Simulates Ctrl+T (new tab), Ctrl+L (address bar focus), types the path, and presses Enter.
3. **Separate Windows** - Last resort. Opens each folder in its own Explorer window.

## Configuration

Configuration is stored in JSON format:

- **Windows**: `%APPDATA%\FileTabOpenerW\config.json`

The config file is compatible with the Python version (file_tab_opener).

## Logging

Logs are written to `%APPDATA%\FileTabOpenerW\debug.log`. Log files are rotated by size (1 MB, up to 3 backups).

## Project Structure

```
FileTabOpenerW/
  CMakeLists.txt
  src/
    main.cpp              # Entry point
    app.h/cpp             # Application lifecycle, dark mode detection
    config.h/cpp          # JSON configuration (nlohmann/json)
    main_window.h/cpp     # Main window with settings bar
    history_section.h/cpp # Folder history with dropdown
    tab_group_section.h/cpp # Tab group management UI
    tab_view.h/cpp        # Custom tab button bar with scrolling
    input_dialog.h/cpp    # Modal input dialog
    explorer_opener.h/cpp # Explorer tab automation (UIA/SendInput)
    i18n.h/cpp            # Internationalization
    utils.h/cpp           # String conversion, path utilities
    logger.h/cpp          # File logger
  res/
    resource.h            # Resource IDs
    app.rc                # Version info, manifest
    app.manifest          # Common controls v6, DPI awareness
  include/
    nlohmann/json.hpp     # JSON library (header-only)
```

## License

[MIT License](LICENSE)

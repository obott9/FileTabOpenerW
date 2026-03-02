# FileTabOpenerW

[English](README.md) | [日本語](README_ja.md) | [한국어](README_ko.md) | [繁體中文](README_zh_TW.md)

用于在 Windows 11+ 的资源管理器中以标签页方式打开文件夹的原生 C++ Win32 应用程序。

这是 [file_tab_opener](https://github.com/obott9/file_tab_opener) (Python/Tk) 的 Windows 原生移植版，使用纯 Win32 API 构建，依赖少，启动速度快。

## 功能

- **标签组管理** - 创建、重命名、复制、删除及排序标签组。复制命名遵循 `"{基础名} {N}"` 模式："工作" → "工作 1" → "工作 2"
- **一键打开** - 将标签组中的所有文件夹在一个资源管理器窗口中以标签页方式打开
- **双布局** - 紧凑布局（默认，标签按钮栏）与侧边栏布局（ListView + 详细面板）；侧边栏布局移植自 macOS SwiftUI 版
- **文件夹历史** - 最近打开的文件夹记录（支持固定）
- **窗口位置** - 按标签组保存及恢复资源管理器的位置和大小
- **多显示器** - 支持多显示器环境的负坐标
- **深色模式** - 自动跟随 Windows 深色/浅色主题
- **单实例** - 同时只运行一个实例。第二次启动会将现有窗口带到前台
- **多语言支持** - 英语、日语、韩语、繁体/简体中文
- **网络路径支持** - UNC 路径（`\\server\share`）即使离线也可注册；需要认证时由资源管理器弹出认证对话框
- **便携式配置** - JSON 配置文件位于 `%APPDATA%\FileTabOpenerW`

## 截图

| 紧凑布局 | 侧边栏布局 |
|:-:|:-:|
| ![Compact](docs/images/zh_CN_compact.png) | ![Sidebar](docs/images/zh_CN_sidebar.png) |

## 下载

从 [GitHub Releases](https://github.com/obott9/FileTabOpenerW/releases) 下载最新的 `.exe`。

> **注意：** 此应用未经代码签名。首次启动时，Windows SmartScreen 可能会显示警告。请点击"更多信息"→"仍要运行"。

## 系统要求

- Windows 11 或更高版本（Windows 10 可能可以运行，但资源管理器标签页功能需要 Win11 22H2+）
- MSVC 构建工具（Visual Studio 2019+ 或 Build Tools for Visual Studio）
- CMake 3.20+

## 构建

### VS Code（推荐）

1. 安装 [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) 扩展
2. 在状态栏选择 **CMake: [Release]**
3. 在状态栏选择构建目标 **[FileTabOpenerW]**
4. 点击状态栏的 **构建** 按钮（或 `Ctrl+Shift+B`）

### 命令行

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

可执行文件将生成于 `build/Release/FileTabOpenerW.exe`。

## 使用方法

1. 启动 `FileTabOpenerW.exe`
2. 使用 **+ 添加标签** 创建标签组
3. 通过路径输入栏或 **浏览...** 添加文件夹路径
4. 根据需要使用 **从资源管理器获取** 设置窗口位置
5. 点击 **以标签打开** 将所有文件夹在资源管理器中以标签页方式打开

### 工作原理

应用程序使用多种策略来打开资源管理器标签页：

1. **UI Automation (UIA)** - 主要方法。使用 Windows UI Automation API 查找资源管理器的"新建标签页"按钮和地址栏，以编程方式创建标签页并导航到各路径。Enter 键通过 PostMessage 直接发送到窗口（非全局按键）。
2. **SendInput** - 备用方法。使用操作系统级按键模拟（Ctrl+T、Ctrl+L、路径输入、Enter）。由于焦点和时序问题，可靠性较低。
3. **独立窗口** - 最后手段。以独立的资源管理器窗口打开每个文件夹。

### 网络路径

支持 UNC 路径（`\\server\share`）。由于网络共享可能需要认证，UNC 路径会跳过常规的目录存在性验证，直接传递给资源管理器。

当 UNC 路径需要认证时：
1. 资源管理器显示 Windows 安全凭据对话框
2. 应用程序不等待认证，继续打开剩余标签页
3. 标签页暂时显示"此电脑"作为占位符
4. 用户完成认证后，资源管理器导航到实际的网络共享

如果用户取消认证对话框，标签页将保留在"此电脑"。

### 性能说明

Windows 资源管理器没有提供标签页操作的公开 API。所有方法都依赖 UI Automation 或按键模拟，每个标签页之间需要等待 UI 响应。打开多个标签页时可能比预期更慢。

> **重要（SendInput 备用方法）：** 打开标签页期间请勿使用键盘或鼠标。SendInput 方法使用操作系统级按键模拟，操作期间的输入可能干扰自动化过程。UIA 方法主要使用定向 UI Automation 和 PostMessage（无全局按键），但当 UIA 操作失败时可能回退到键盘输入。

## 配置

配置以 JSON 格式保存：

- **Windows**: `%APPDATA%\FileTabOpenerW\config.json`

配置文件与 Python 版（file_tab_opener）兼容。

## 日志

日志输出至 `%APPDATA%\FileTabOpenerW\debug.log`。日志文件按大小进行轮替（1 MB，最多 3 个备份）。

## 项目结构

```
FileTabOpenerW/
  CMakeLists.txt
  src/
    main.cpp              # 入口点
    app.h/cpp             # 应用程序生命周期、深色模式检测
    config.h/cpp          # JSON 配置 (nlohmann/json)
    main_window.h/cpp     # 主窗口（含设置栏）
    history_section.h/cpp # 文件夹历史（含下拉菜单）
    tab_group_section.h/cpp # 标签组管理 UI（紧凑布局）
    modern_tab_group_section.h/cpp # 侧边栏布局（ListView + 详细面板）
    tab_view.h/cpp        # 自定义标签按钮栏（支持滚动）
    theme.h               # 颜色主题常量
    input_dialog.h/cpp    # 模态输入对话框
    explorer_opener.h/cpp # 资源管理器标签页自动化 (UIA/SendInput)
    i18n.h/cpp            # 多语言支持
    utils.h/cpp           # 字符串转换、路径工具
    logger.h/cpp          # 文件日志器
  res/
    resource.h            # 资源 ID
    app.rc                # 版本信息、清单
    app.manifest          # Common Controls v6、DPI 感知
  include/
    nlohmann/json.hpp     # JSON 库（仅头文件）
```

## 作者

[obott9](https://github.com/obott9)

## 相关项目

- **[file_tab_opener](https://github.com/obott9/file_tab_opener)** — 跨平台版（Python/Tk）。支持 macOS 和 Windows。
- **[FileTabOpenerM](https://github.com/obott9/FileTabOpenerM)** — macOS 原生版（Swift/SwiftUI）。AX API + AppleScript 混合方式控制 Finder 标签页。

## 开发

本项目与 Anthropic 的 **Claude AI** 协作开发。
Claude 提供了以下支持：
* 架构设计与代码实现
* 多语言本地化
* 文档与 README 编写

## 支持

如果您觉得这个应用有用，请在 GitHub 上给它一颗星！

[![GitHub Stars](https://img.shields.io/github/stars/obott9/FileTabOpenerW?style=social)](https://github.com/obott9/FileTabOpenerW)

也欢迎请我喝杯咖啡或成为赞助者！

[![GitHub Sponsors](https://img.shields.io/badge/GitHub%20Sponsors-♥-ea4aaa)](https://github.com/sponsors/obott9)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-☕-orange)](https://buymeacoffee.com/obott9)

## 许可证

[MIT License](LICENSE)

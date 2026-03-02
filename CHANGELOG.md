# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [1.0.0] - 2026-03-03

Initial release of File Tab Opener (Windows Native).
Native C++/Win32 reimplementation of [file_tab_opener](https://github.com/obott9/file_tab_opener) (Python/Tk v1.1.3).

### Added
- **Core**: UI Automation + SendInput + separate windows — 3-tier fallback for Explorer tab control
- **Core**: Single-instance guard (Mutex) — second launch brings existing window to foreground
- **Core**: BstrGuard RAII wrapper for safe COM BSTR management
- **UI**: Compact layout (Python version compatible) with tab button bar and path list
- **UI**: Sidebar layout with ListView sidebar + detail panel
- **UI**: Compact/Sidebar layout toggle with owner-drawn themed buttons
- **UI**: Dark mode support (follows Windows system theme automatically)
- **UI**: Toast-style progress indicator during tab opening
- **UI**: DPI-aware rendering (PerMonitorV2 manifest)
- **Tab Groups**: Create, rename, copy, delete, and reorder tab groups
- **Tab Groups**: Window geometry (position/size) save and restore per tab group
- **Tab Groups**: Multi-monitor support with negative coordinates
- **Paths**: Add, remove, reorder paths within a tab group
- **Paths**: Browse dialog for folder selection
- **History**: Recently opened folders with pin support
- **i18n**: 5 languages (English, Japanese, Korean, Traditional Chinese, Simplified Chinese)
- **Config**: JSON configuration compatible with Python version (snake_case keys)
- **Config**: nlohmann/json header-only library (vendored)
- **Logging**: File logger at `%APPDATA%\FileTabOpenerW\debug.log`
- **Logging**: Log rotation (1 MB threshold, 3 backup generations)
- **Docs**: README in 5 languages, MIT License
- **Docs**: Screenshots per language (5 languages x 2 layouts = 10 screenshots)

[Unreleased]: https://github.com/obott9/FileTabOpenerW/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/obott9/FileTabOpenerW/releases/tag/v1.0.0

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <tuple>

namespace fto {

struct WindowRect {
    int x, y, width, height;
};

// Callback types
using ProgressCallback = std::function<void(int current, int total, const std::wstring& path)>;
using ErrorCallback = std::function<void(const std::wstring& path, const std::wstring& error)>;

// Open a single folder in a new Explorer window
bool open_single_folder(const std::wstring& path,
                        const std::optional<WindowRect>& rect = std::nullopt,
                        float timeout = 30.0f);

// Open multiple folders as tabs in a single Explorer window
// 3-tier fallback: UIA -> SendInput -> separate windows
bool open_folders_as_tabs(const std::vector<std::wstring>& paths,
                          const ProgressCallback& on_progress = nullptr,
                          const ErrorCallback& on_error = nullptr,
                          float timeout = 30.0f,
                          const std::optional<WindowRect>& rect = std::nullopt);

// Get the frontmost Explorer window rect (x, y, w, h), or nullopt
std::optional<WindowRect> get_frontmost_explorer_rect();

} // namespace fto

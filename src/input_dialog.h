#pragma once
#include <string>
#include <optional>
#include <windows.h>

namespace fto {

// Show a modal text input dialog.
// Returns the entered text, or nullopt if cancelled.
std::optional<std::wstring> show_input_dialog(
    HWND parent,
    const std::wstring& title,
    const std::wstring& prompt,
    const std::wstring& initial_value = L"");

} // namespace fto

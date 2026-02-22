#pragma once
#include <string>
#include <vector>
#include <utility>

namespace fto {

// String conversion (UTF-8 <-> UTF-16)
std::wstring utf8_to_wide(const std::string& s);
std::string wide_to_utf8(const std::wstring& s);

// Path utilities
bool is_unc_path(const std::wstring& path);
bool is_directory(const std::wstring& path);
std::wstring normalize_path(const std::wstring& path);
std::wstring expand_user(const std::wstring& path);
std::wstring strip_quotes(const std::wstring& text);

// Validate paths: returns (valid, invalid)
std::pair<std::vector<std::wstring>, std::vector<std::wstring>>
validate_paths(const std::vector<std::wstring>& paths);

// Get config directory (%APPDATA%\FileTabOpenerW)
std::wstring get_appdata_dir();

// Get current ISO timestamp (seconds precision)
std::string get_iso_timestamp();

} // namespace fto

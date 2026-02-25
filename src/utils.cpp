#include "utils.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fto {

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0) return {};
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), result.data(), len);
    return result;
}

std::string wide_to_utf8(const std::wstring& s) {
    if (s.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), result.data(), len, nullptr, nullptr);
    return result;
}

bool is_unc_path(const std::wstring& path) {
    std::wstring normalized = path;
    for (auto& c : normalized) {
        if (c == L'/') c = L'\\';
    }
    return normalized.size() >= 2 && normalized[0] == L'\\' && normalized[1] == L'\\';
}

bool is_directory(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring normalize_path(const std::wstring& path) {
    try {
        auto canonical = std::filesystem::weakly_canonical(path);
        return canonical.wstring();
    } catch (...) {
        // Fallback for paths that can't be canonicalized (e.g., UNC)
        wchar_t buf[MAX_PATH];
        if (PathCanonicalizeW(buf, path.c_str())) {
            return buf;
        }
        return path;
    }
}

std::wstring expand_user(const std::wstring& path) {
    if (path.empty() || path[0] != L'~') return path;
    PWSTR home = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &home))) {
        std::wstring result = std::wstring(home) + path.substr(1);
        CoTaskMemFree(home);
        return result;
    }
    return path;
}

std::wstring strip_quotes(const std::wstring& text) {
    if (text.size() >= 2) {
        wchar_t first = text.front(), last = text.back();
        if ((first == L'"' && last == L'"') || (first == L'\'' && last == L'\'')) {
            return text.substr(1, text.size() - 2);
        }
    }
    return text;
}

std::pair<std::vector<std::wstring>, std::vector<std::wstring>>
validate_paths(const std::vector<std::wstring>& paths) {
    std::vector<std::wstring> valid, invalid;
    for (const auto& p : paths) {
        std::wstring expanded = expand_user(p);
        if (is_unc_path(expanded) || is_directory(expanded)) {
            valid.push_back(expanded);
        } else {
            invalid.push_back(p);
        }
    }
    return {valid, invalid};
}

std::wstring get_appdata_dir() {
    wchar_t buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf))) {
        return std::wstring(buf) + L"\\FileTabOpenerW";
    }
    return L".\\FileTabOpenerW";
}

std::string get_iso_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_s(&tm_buf, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

} // namespace fto

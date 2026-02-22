#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace fto {

// Supported languages
constexpr const wchar_t* LANG_EN    = L"en";
constexpr const wchar_t* LANG_JA    = L"ja";
constexpr const wchar_t* LANG_KO    = L"ko";
constexpr const wchar_t* LANG_ZH_TW = L"zh_TW";
constexpr const wchar_t* LANG_ZH_CN = L"zh_CN";

struct LangInfo {
    const wchar_t* code;
    const wchar_t* display_name;
};

inline const std::vector<LangInfo>& supported_langs() {
    static const std::vector<LangInfo> langs = {
        {LANG_EN,    L"English"},
        {LANG_JA,    L"\x65E5\x672C\x8A9E"},       // 日本語
        {LANG_KO,    L"\xD55C\xAD6D\xC5B4"},       // 한국어
        {LANG_ZH_TW, L"\x7E41\x9AD4\x4E2D\x6587"}, // 繁體中文
        {LANG_ZH_CN, L"\x7B80\x4F53\x4E2D\x6587"}, // 简体中文
    };
    return langs;
}

// Initialize i18n (detect system language)
void i18n_init();

// Set/get current language
void set_language(const std::wstring& lang);
std::wstring get_language();

// Detect system language
std::wstring detect_system_language();

// Get translated string by key
// Supports {name} placeholders via kwargs
std::wstring t(const std::string& key);
std::wstring t(const std::string& key,
               const std::unordered_map<std::string, std::wstring>& kwargs);

} // namespace fto

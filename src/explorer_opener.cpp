#include "explorer_opener.h"
#include "utils.h"
#include "logger.h"
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <uiautomation.h>
#include <wrl/client.h>
#include <thread>
#include <chrono>
#include <set>
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace fto {

// --- Win32 constants ---
static constexpr WORD VK_CONTROL_KEY = VK_CONTROL;
static constexpr WORD VK_T_KEY = 0x54;
static constexpr WORD VK_L_KEY = 0x4C;
static constexpr WORD VK_RETURN_KEY = VK_RETURN;

// --- Keyboard input helpers ---

static void send_key_combo(WORD vk_mod, WORD vk_key) {
    INPUT inputs[4] = {};
    for (auto& inp : inputs) inp.type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk_mod;
    inputs[1].ki.wVk = vk_key;
    inputs[2].ki.wVk = vk_key;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].ki.wVk = vk_mod;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

static void type_string(const std::wstring& text) {
    for (wchar_t ch : text) {
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wScan = ch;
        inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wScan = ch;
        inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(2, inputs, sizeof(INPUT));
    }
}

static void press_key(WORD vk) {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

// --- Window utilities ---

static std::vector<HWND> enum_explorer_hwnds() {
    std::vector<HWND> hwnds;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t cls[256];
        GetClassNameW(hwnd, cls, 256);
        if (wcscmp(cls, L"CabinetWClass") == 0) {
            reinterpret_cast<std::vector<HWND>*>(lParam)->push_back(hwnd);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&hwnds));
    return hwnds;
}

static HWND find_new_explorer_hwnd(const std::set<HWND>& before, float timeout_sec) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeout_sec) break;
        for (HWND hwnd : enum_explorer_hwnds()) {
            if (before.find(hwnd) == before.end()) {
                LOG_DEBUG("explorer", "New Explorer window: hwnd=%p (%.1fs)", hwnd, elapsed);
                return hwnd;
            }
        }
        Sleep(200);
    }
    LOG_WARN("explorer", "New Explorer window not found (timeout=%.1fs)", timeout_sec);
    return nullptr;
}

static void bring_to_foreground(HWND hwnd) {
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
}

static void apply_window_rect(HWND hwnd, const WindowRect& rect) {
    MoveWindow(hwnd, rect.x, rect.y, rect.width, rect.height, TRUE);
    LOG_DEBUG("explorer", "Applied rect: x=%d y=%d w=%d h=%d", rect.x, rect.y, rect.width, rect.height);
}

static void post_enter_key(HWND hwnd) {
    PostMessageW(hwnd, WM_KEYDOWN, VK_RETURN_KEY, 0);
    Sleep(50);
    PostMessageW(hwnd, WM_KEYUP, VK_RETURN_KEY, 0);
}

static bool is_explorer_hwnd(HWND hwnd) {
    wchar_t cls[256];
    GetClassNameW(hwnd, cls, 256);
    return wcscmp(cls, L"CabinetWClass") == 0;
}

// --- UIA method ---

static bool open_tabs_uia(
    const std::vector<std::wstring>& paths,
    const ProgressCallback& on_progress,
    const ErrorCallback& on_error,
    float timeout,
    const std::optional<WindowRect>& rect)
{
    LOG_INFO("explorer", "Using UIA method");

    ComPtr<IUIAutomation> uia;
    HRESULT hr = CoCreateInstance(__uuidof(CUIAutomation), nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&uia));
    if (FAILED(hr)) {
        LOG_WARN("explorer", "CoCreateInstance(IUIAutomation) failed: 0x%08X", hr);
        return false;
    }

    // Record existing windows
    auto before_hwnds_vec = enum_explorer_hwnds();
    std::set<HWND> before_set(before_hwnds_vec.begin(), before_hwnds_vec.end());

    // Open first path
    std::wstring first_path = normalize_path(paths[0]);
    ShellExecuteW(nullptr, L"explore", first_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    HWND new_hwnd = find_new_explorer_hwnd(before_set, timeout);
    if (!new_hwnd) return false;

    if (on_progress) on_progress(1, (int)paths.size(), paths[0]);
    if (paths.size() == 1) {
        if (rect) apply_window_rect(new_hwnd, *rect);
        return true;
    }

    // Connect via UIA
    ComPtr<IUIAutomationElement> win_elem;
    hr = uia->ElementFromHandle(new_hwnd, &win_elem);
    if (FAILED(hr) || !win_elem) {
        LOG_WARN("explorer", "ElementFromHandle failed");
        return false;
    }

    bring_to_foreground(new_hwnd);
    Sleep(500);

    // Find AddButton
    ComPtr<IUIAutomationElement> add_btn;
    {
        VARIANT prop;
        VariantInit(&prop);
        prop.vt = VT_BSTR;
        prop.bstrVal = SysAllocString(L"AddButton");
        ComPtr<IUIAutomationCondition> cond;
        uia->CreatePropertyCondition(UIA_AutomationIdPropertyId, prop, &cond);
        SysFreeString(prop.bstrVal);
        if (cond) win_elem->FindFirst(TreeScope_Descendants, cond.Get(), &add_btn);
    }
    if (add_btn) LOG_DEBUG("explorer", "Found AddButton via UIA");

    // Find XAML host for PostMessage Enter
    HWND xaml_hwnd = nullptr;
    {
        HWND child = FindWindowExW(new_hwnd, nullptr, L"InputSiteWindowClass", nullptr);
        if (child) {
            xaml_hwnd = child;
            LOG_DEBUG("explorer", "Found InputSiteWindowClass: %p", xaml_hwnd);
        }
    }

    // Open remaining tabs
    for (int i = 1; i < (int)paths.size(); ++i) {
        std::wstring norm_path = normalize_path(paths[i]);
        LOG_DEBUG("explorer", "Adding tab [%d/%d]: %s", i + 1, (int)paths.size(),
                  wide_to_utf8(norm_path).c_str());

        // New tab: UIA Invoke or Ctrl+T
        bool tab_created = false;
        if (add_btn) {
            ComPtr<IUIAutomationInvokePattern> invoke;
            if (SUCCEEDED(add_btn->GetCurrentPatternAs(UIA_InvokePatternId,
                          IID_PPV_ARGS(&invoke))) && invoke) {
                if (SUCCEEDED(invoke->Invoke())) {
                    tab_created = true;
                    LOG_DEBUG("explorer", "New tab via UIA Invoke");
                }
            }
        }
        if (!tab_created) {
            send_key_combo(VK_CONTROL_KEY, VK_T_KEY);
        }
        Sleep(800);

        // Focus address bar and set path (with retry)
        bool path_set = false;
        for (int attempt = 0; attempt < 3 && !path_set; ++attempt) {
            // Find address bar Edit
            ComPtr<IUIAutomationElement> auto_suggest;
            {
                VARIANT prop;
                VariantInit(&prop);
                prop.vt = VT_BSTR;
                prop.bstrVal = SysAllocString(L"PART_AutoSuggestBox");
                ComPtr<IUIAutomationCondition> cond;
                uia->CreatePropertyCondition(UIA_AutomationIdPropertyId, prop, &cond);
                SysFreeString(prop.bstrVal);
                if (cond) win_elem->FindFirst(TreeScope_Descendants, cond.Get(), &auto_suggest);
            }

            ComPtr<IUIAutomationElement> addr_edit;
            if (auto_suggest) {
                VARIANT prop;
                VariantInit(&prop);
                prop.vt = VT_BSTR;
                prop.bstrVal = SysAllocString(L"TextBox");
                ComPtr<IUIAutomationCondition> cond;
                uia->CreatePropertyCondition(UIA_AutomationIdPropertyId, prop, &cond);
                SysFreeString(prop.bstrVal);
                if (cond) auto_suggest->FindFirst(TreeScope_Descendants, cond.Get(), &addr_edit);
            }

            if (addr_edit) {
                // Try UIA focus
                addr_edit->SetFocus();
                Sleep(200);

                // Set value via ValuePattern
                ComPtr<IUIAutomationValuePattern> value_pat;
                if (SUCCEEDED(addr_edit->GetCurrentPatternAs(UIA_ValuePatternId,
                              IID_PPV_ARGS(&value_pat))) && value_pat) {
                    BSTR bstr_path = SysAllocString(norm_path.c_str());
                    if (SUCCEEDED(value_pat->SetValue(bstr_path))) {
                        LOG_DEBUG("explorer", "Path set via UIA ValuePattern (attempt %d)", attempt + 1);
                        path_set = true;
                    }
                    SysFreeString(bstr_path);
                }
            }

            if (!path_set) {
                // Fallback: Ctrl+L + keyboard input
                send_key_combo(VK_CONTROL_KEY, VK_L_KEY);
                Sleep(300);
                type_string(norm_path);
                path_set = true;
                LOG_DEBUG("explorer", "Path set via keyboard (attempt %d)", attempt + 1);
            }
        }

        Sleep(100);

        // Navigate: Enter
        if (xaml_hwnd) {
            post_enter_key(xaml_hwnd);
            LOG_DEBUG("explorer", "Enter via PostMessage");
        } else {
            press_key(VK_RETURN_KEY);
        }

        // Wait for navigation (poll address bar focus loss)
        Sleep(1000);
        // Simple timeout-based wait
        auto nav_start = std::chrono::steady_clock::now();
        while (std::chrono::duration<float>(
                   std::chrono::steady_clock::now() - nav_start).count() < timeout) {
            // Check if address bar still has focus by trying to find it
            ComPtr<IUIAutomationElement> focused;
            uia->GetFocusedElement(&focused);
            if (focused) {
                BSTR auto_id = nullptr;
                focused->get_CurrentAutomationId(&auto_id);
                bool is_addr = false;
                if (auto_id) {
                    is_addr = (wcscmp(auto_id, L"TextBox") == 0);
                    SysFreeString(auto_id);
                }
                if (!is_addr) {
                    LOG_DEBUG("explorer", "Navigation complete");
                    Sleep(300);
                    break;
                }
            }
            Sleep(100);
        }

        if (on_progress) on_progress(i + 1, (int)paths.size(), paths[i]);
    }

    if (rect) apply_window_rect(new_hwnd, *rect);
    bring_to_foreground(new_hwnd);
    return true;
}

// --- SendInput method ---

static bool open_tabs_sendinput(
    const std::vector<std::wstring>& paths,
    const ProgressCallback& on_progress,
    const ErrorCallback& on_error,
    float timeout,
    const std::optional<WindowRect>& rect)
{
    LOG_INFO("explorer", "Using SendInput method");

    auto before_hwnds_vec = enum_explorer_hwnds();
    std::set<HWND> before_set(before_hwnds_vec.begin(), before_hwnds_vec.end());

    ShellExecuteW(nullptr, L"explore", normalize_path(paths[0]).c_str(),
                  nullptr, nullptr, SW_SHOWNORMAL);
    Sleep(1500);

    if (on_progress) on_progress(1, (int)paths.size(), paths[0]);

    HWND new_hwnd = find_new_explorer_hwnd(before_set, timeout);
    if (!new_hwnd) return false;

    if (paths.size() == 1) {
        if (rect) apply_window_rect(new_hwnd, *rect);
        return true;
    }

    bring_to_foreground(new_hwnd);
    Sleep(300);

    for (int i = 1; i < (int)paths.size(); ++i) {
        std::wstring norm_path = normalize_path(paths[i]);
        LOG_DEBUG("explorer", "SendInput [%d/%d]: %s", i + 1, (int)paths.size(),
                  wide_to_utf8(norm_path).c_str());
        send_key_combo(VK_CONTROL_KEY, VK_T_KEY);
        Sleep(500);
        send_key_combo(VK_CONTROL_KEY, VK_L_KEY);
        Sleep(300);
        type_string(norm_path);
        Sleep(100);
        press_key(VK_RETURN_KEY);
        Sleep(800);
        if (on_progress) on_progress(i + 1, (int)paths.size(), paths[i]);
    }

    if (rect) apply_window_rect(new_hwnd, *rect);
    return true;
}

// --- Separate windows fallback ---

static bool open_tabs_separate(
    const std::vector<std::wstring>& paths,
    const ProgressCallback& on_progress,
    const ErrorCallback& on_error,
    float timeout,
    const std::optional<WindowRect>& rect)
{
    LOG_INFO("explorer", "Opening as separate windows (fallback)");
    for (int i = 0; i < (int)paths.size(); ++i) {
        std::wstring norm = normalize_path(paths[i]);
        auto before_vec = enum_explorer_hwnds();
        std::set<HWND> before(before_vec.begin(), before_vec.end());
        ShellExecuteW(nullptr, L"explore", norm.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (rect) {
            HWND hwnd = find_new_explorer_hwnd(before, timeout);
            if (hwnd) apply_window_rect(hwnd, *rect);
        }
        if (on_progress) on_progress(i + 1, (int)paths.size(), paths[i]);
        Sleep(300);
    }
    return true;
}

// --- Public API ---

bool open_single_folder(const std::wstring& path,
                        const std::optional<WindowRect>& rect, float timeout) {
    std::wstring norm = normalize_path(path);
    auto before_vec = enum_explorer_hwnds();
    std::set<HWND> before(before_vec.begin(), before_vec.end());
    ShellExecuteW(nullptr, L"explore", norm.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (rect) {
        HWND hwnd = find_new_explorer_hwnd(before, timeout);
        if (hwnd) apply_window_rect(hwnd, *rect);
    }
    return true;
}

bool open_folders_as_tabs(const std::vector<std::wstring>& paths,
                          const ProgressCallback& on_progress,
                          const ErrorCallback& on_error,
                          float timeout,
                          const std::optional<WindowRect>& rect)
{
    if (paths.empty()) return false;

    // Deduplicate (normalize before comparison to catch path variants)
    std::vector<std::wstring> unique_paths;
    std::set<std::wstring> seen;
    for (const auto& p : paths) {
        std::wstring norm = normalize_path(p);
        if (seen.insert(norm).second) unique_paths.push_back(p);
    }

    LOG_INFO("explorer", "Opening as tabs: %d paths, timeout=%.0fs",
             (int)unique_paths.size(), timeout);

    // Try UIA
    try {
        if (open_tabs_uia(unique_paths, on_progress, on_error, timeout, rect))
            return true;
    } catch (...) {
        LOG_WARN("explorer", "UIA method failed with exception");
    }

    // Try SendInput
    try {
        if (open_tabs_sendinput(unique_paths, on_progress, on_error, timeout, rect))
            return true;
    } catch (...) {
        LOG_WARN("explorer", "SendInput method failed with exception");
    }

    // Fallback: separate windows
    return open_tabs_separate(unique_paths, on_progress, on_error, timeout, rect);
}

std::optional<WindowRect> get_frontmost_explorer_rect() {
    HWND fg = GetForegroundWindow();
    HWND target = nullptr;
    if (fg && is_explorer_hwnd(fg)) {
        target = fg;
    } else {
        auto hwnds = enum_explorer_hwnds();
        if (hwnds.empty()) return std::nullopt;
        target = hwnds[0];
    }

    RECT r;
    if (!GetWindowRect(target, &r)) return std::nullopt;
    return WindowRect{r.left, r.top, r.right - r.left, r.bottom - r.top};
}

} // namespace fto

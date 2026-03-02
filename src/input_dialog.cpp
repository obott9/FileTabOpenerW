#include "input_dialog.h"
#include "theme.h"
#include "res/resource.h"
#include <commctrl.h>

namespace fto {

struct InputDialogData {
    const std::wstring* title;
    const std::wstring* prompt;
    const std::wstring* initial;
    std::wstring result;
    bool ok = false;
    HFONT font = nullptr;
};

static LRESULT CALLBACK InputDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* data = reinterpret_cast<InputDialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        data = reinterpret_cast<InputDialogData*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

        data->font = create_system_message_font();
        HFONT hFont = data->font;

        // Prompt label
        HWND label = CreateWindowW(L"STATIC", data->prompt->c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 10, 360, 20, hwnd, nullptr, nullptr, nullptr);
        SendMessageW(label, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Edit control
        HWND edit = CreateWindowW(L"EDIT", data->initial->c_str(),
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 35, 360, 24, hwnd, (HMENU)(INT_PTR)IDC_INPUT_EDIT, nullptr, nullptr);
        SendMessageW(edit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(edit, EM_SETSEL, 0, -1);

        // OK button
        HWND ok_btn = CreateWindowW(L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            200, 70, 80, 28, hwnd, (HMENU)(INT_PTR)IDC_INPUT_OK, nullptr, nullptr);
        SendMessageW(ok_btn, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Cancel button
        HWND cancel_btn = CreateWindowW(L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            290, 70, 80, 28, hwnd, (HMENU)(INT_PTR)IDC_INPUT_CANCEL, nullptr, nullptr);
        SendMessageW(cancel_btn, WM_SETFONT, (WPARAM)hFont, TRUE);

        SetFocus(edit);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_INPUT_OK) {
            wchar_t buf[1024];
            GetDlgItemTextW(hwnd, IDC_INPUT_EDIT, buf, 1024);
            data->result = buf;
            data->ok = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_INPUT_CANCEL) {
            data->ok = false;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            data->ok = false;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_DESTROY: {
        if (data && data->font) {
            DeleteObject(data->font);
            data->font = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::optional<std::wstring> show_input_dialog(
    HWND parent, const std::wstring& title,
    const std::wstring& prompt, const std::wstring& initial_value)
{
    static bool registered = false;
    static const wchar_t* cls_name = L"FTOInputDialog";

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = InputDlgProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszClassName = cls_name;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        registered = true;
    }

    InputDialogData data;
    data.title = &title;
    data.prompt = &prompt;
    data.initial = &initial_value;

    // Center on parent
    RECT pr;
    GetWindowRect(parent, &pr);
    int dlg_w = 400, dlg_h = 140;
    int x = pr.left + (pr.right - pr.left - dlg_w) / 2;
    int y = pr.top + (pr.bottom - pr.top - dlg_h) / 2;

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        cls_name, title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlg_w, dlg_h,
        parent, nullptr, GetModuleHandleW(nullptr), &data);

    EnableWindow(parent, FALSE);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        // Handle Escape regardless of focus
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            data.ok = false;
            DestroyWindow(dlg);
            continue;
        }
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);

    if (data.ok) return data.result;
    return std::nullopt;
}

} // namespace fto

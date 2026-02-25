#pragma once
#include <windows.h>

namespace fto {
namespace theme {

// === CTkButton "blue" theme colors ===

// Standard button (light mode)
constexpr COLORREF BTN_BG_LIGHT       = RGB(0x3B, 0x8E, 0xD0);  // #3B8ED0
constexpr COLORREF BTN_PRESSED_LIGHT  = RGB(0x36, 0x71, 0x9F);  // #36719F
constexpr COLORREF BTN_TEXT_LIGHT     = RGB(0xFF, 0xFF, 0xFF);  // white

// Standard button (dark mode)
constexpr COLORREF BTN_BG_DARK        = RGB(0x1F, 0x6A, 0xA5);  // #1F6AA5
constexpr COLORREF BTN_PRESSED_DARK   = RGB(0x14, 0x48, 0x70);  // #144870
constexpr COLORREF BTN_TEXT_DARK      = RGB(0xFF, 0xFF, 0xFF);  // white

// Tab unselected (light): gray78 bg, gray20 text
constexpr COLORREF TAB_UNSEL_BG_LIGHT      = RGB(0xC7, 0xC7, 0xC7);
constexpr COLORREF TAB_UNSEL_PRESSED_LIGHT = RGB(0xB0, 0xB0, 0xB0);
constexpr COLORREF TAB_UNSEL_TEXT_LIGHT    = RGB(0x33, 0x33, 0x33);

// Tab unselected (dark): gray28 bg, gray80 text
constexpr COLORREF TAB_UNSEL_BG_DARK       = RGB(0x47, 0x47, 0x47);
constexpr COLORREF TAB_UNSEL_PRESSED_DARK  = RGB(0x3A, 0x3A, 0x3A);
constexpr COLORREF TAB_UNSEL_TEXT_DARK     = RGB(0xCC, 0xCC, 0xCC);

// Parent background (for rounded corner fill)
constexpr COLORREF PARENT_BG_DARK = RGB(0x2B, 0x2B, 0x2B);

// Toast (light)
constexpr COLORREF TOAST_BG_LIGHT     = RGB(0xE0, 0xE0, 0xE0);
constexpr COLORREF TOAST_FG_LIGHT     = RGB(0x33, 0x33, 0x33);
constexpr COLORREF TOAST_BORDER_LIGHT = RGB(0xBB, 0xBB, 0xBB);

// Toast (dark)
constexpr COLORREF TOAST_BG_DARK      = RGB(0x2B, 0x2B, 0x2B);
constexpr COLORREF TOAST_FG_DARK      = RGB(0xFF, 0xFF, 0xFF);
constexpr COLORREF TOAST_BORDER_DARK  = RGB(0x55, 0x55, 0x55);

// Listbox selection
constexpr COLORREF LIST_SELECT_BG = RGB(0x1F, 0x6A, 0xA5);

// Corner radius for rounded buttons (matching CTkButton)
constexpr int CORNER_RADIUS = 6;

} // namespace theme

/// Create a font from the system message font (NONCLIENTMETRICS.lfMessageFont).
/// Returns an HFONT that the caller must DeleteObject when done.
/// This is the standard Win11 UI font (e.g., "Yu Gothic UI" 9pt on Japanese systems).
inline HFONT create_system_message_font() {
    NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    return CreateFontIndirectW(&ncm.lfMessageFont);
}

/// Draw an owner-drawn button with themed colors.
/// parent_bg is used to fill corners when corner_radius > 0.
inline void draw_themed_button(const DRAWITEMSTRUCT* dis,
                               COLORREF bg, COLORREF bg_pressed,
                               COLORREF text_color, COLORREF parent_bg,
                               int corner_radius = theme::CORNER_RADIUS) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    COLORREF fill = (dis->itemState & ODS_SELECTED) ? bg_pressed : bg;

    // Fill parent bg first (for transparent rounded corners)
    if (corner_radius > 0) {
        HBRUSH pbr = CreateSolidBrush(parent_bg);
        FillRect(hdc, &rc, pbr);
        DeleteObject(pbr);
    }

    // Draw button shape
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, fill);
    HBRUSH old_brush = (HBRUSH)SelectObject(hdc, brush);
    HPEN old_pen = (HPEN)SelectObject(hdc, pen);

    if (corner_radius > 0) {
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom,
                  corner_radius, corner_radius);
    } else {
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);

    // Draw text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text_color);

    wchar_t text[256];
    GetWindowTextW(dis->hwndItem, text, 256);

    HFONT font = (HFONT)SendMessageW(dis->hwndItem, WM_GETFONT, 0, 0);
    HFONT old_font = font ? (HFONT)SelectObject(hdc, font) : nullptr;
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (old_font) SelectObject(hdc, old_font);

    // Focus rect
    if (dis->itemState & ODS_FOCUS) {
        RECT frc = rc;
        InflateRect(&frc, -3, -3);
        DrawFocusRect(hdc, &frc);
    }
}

} // namespace fto

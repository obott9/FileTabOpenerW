#include "i18n.h"
#include "utils.h"
#include "logger.h"
#include <windows.h>
#include <algorithm>

namespace fto {

static std::wstring g_current_lang = L"en";

// Translation table: key -> {lang -> text}
struct Entry { const char* key; const wchar_t* lang; const wchar_t* text; };

// clang-format off
static const Entry g_strings[] = {
    // --- App ---
    {"app.title", L"en", L"FileTabOpenerW"},
    {"app.title", L"ja", L"FileTabOpenerW"},
    {"app.title", L"ko", L"FileTabOpenerW"},
    {"app.title", L"zh_TW", L"FileTabOpenerW"},
    {"app.title", L"zh_CN", L"FileTabOpenerW"},

    // --- History section ---
    {"history.label", L"en", L"History:"},
    {"history.label", L"ja", L"\x5C65\x6B74:"},
    {"history.label", L"ko", L"\xAE30\xB85D:"},
    {"history.label", L"zh_TW", L"\x6B77\x53F2:"},
    {"history.label", L"zh_CN", L"\x5386\x53F2:"},

    {"history.open_explorer", L"en", L"Open in Explorer"},
    {"history.open_explorer", L"ja", L"\x30A8\x30AF\x30B9\x30D7\x30ED\x30FC\x30E9\x30FC\x3067\x958B\x304F"},
    {"history.open_explorer", L"ko", L"\xD0D0\xC0C9\xAE30\xC5D0\xC11C \xC5F4\xAE30"},
    {"history.open_explorer", L"zh_TW", L"\x5728\x6A94\x6848\x7E3D\x7BA1\x4E2D\x958B\x555F"},
    {"history.open_explorer", L"zh_CN", L"\x5728\x8D44\x6E90\x7BA1\x7406\x5668\x4E2D\x6253\x5F00"},

    {"history.pin", L"en", L"Pin"},
    {"history.pin", L"ja", L"Pin"},
    {"history.pin", L"ko", L"Pin"},
    {"history.pin", L"zh_TW", L"Pin"},
    {"history.pin", L"zh_CN", L"Pin"},

    {"history.clear", L"en", L"Clear"},
    {"history.clear", L"ja", L"Clear"},
    {"history.clear", L"ko", L"Clear"},
    {"history.clear", L"zh_TW", L"Clear"},
    {"history.clear", L"zh_CN", L"Clear"},

    {"history.clear_confirm_title", L"en", L"Clear History"},
    {"history.clear_confirm_title", L"ja", L"\x5C65\x6B74\x30AF\x30EA\x30A2"},
    {"history.clear_confirm_title", L"ko", L"\xAE30\xB85D \xC9C0\xC6B0\xAE30"},
    {"history.clear_confirm_title", L"zh_TW", L"\x6E05\x9664\x6B77\x53F2"},
    {"history.clear_confirm_title", L"zh_CN", L"\x6E05\x9664\x5386\x53F2"},

    {"history.clear_confirm_msg", L"en", L"Delete all history except pinned items?"},
    {"history.clear_confirm_msg", L"ja", L"\x30D4\x30F3\x7559\x3081\x4EE5\x5916\x306E\x5C65\x6B74\x3092\x3059\x3079\x3066\x524A\x9664\x3057\x307E\x3059\x304B\xFF1F"},
    {"history.clear_confirm_msg", L"ko", L"\xACE0\xC815\xB41C \xD56D\xBAA9\xC744 \xC81C\xC678\xD55C \xBAA8\xB4E0 \xAE30\xB85D\xC744 \xC0AD\xC81C\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?"},
    {"history.clear_confirm_msg", L"zh_TW", L"\x662F\x5426\x522A\x9664\x6240\x6709\x672A\x91D8\x9078\x7684\x6B77\x53F2\x8A18\x9304\xFF1F"},
    {"history.clear_confirm_msg", L"zh_CN", L"\x662F\x5426\x5220\x9664\x6240\x6709\x672A\x56FA\x5B9A\x7684\x5386\x53F2\x8BB0\x5F55\xFF1F"},

    {"history.invalid_path_title", L"en", L"Invalid Path"},
    {"history.invalid_path_title", L"ja", L"\x7121\x52B9\x306A\x30D1\x30B9"},
    {"history.invalid_path_title", L"ko", L"\xC798\xBABB\xB41C \xACBD\xB85C"},
    {"history.invalid_path_title", L"zh_TW", L"\x7121\x6548\x8DEF\x5F91"},
    {"history.invalid_path_title", L"zh_CN", L"\x65E0\x6548\x8DEF\x5F84"},

    {"history.invalid_path_msg", L"en", L"Folder does not exist:\n{path}"},
    {"history.invalid_path_msg", L"ja", L"\x30D5\x30A9\x30EB\x30C0\x304C\x5B58\x5728\x3057\x307E\x305B\x3093:\n{path}"},
    {"history.invalid_path_msg", L"ko", L"\xD3F4\xB354\xAC00 \xC874\xC7AC\xD558\xC9C0 \xC54A\xC2B5\xB2C8\xB2E4:\n{path}"},
    {"history.invalid_path_msg", L"zh_TW", L"\x8CC7\x6599\x593E\x4E0D\x5B58\x5728\xFF1A\n{path}"},
    {"history.invalid_path_msg", L"zh_CN", L"\x6587\x4EF6\x5939\x4E0D\x5B58\x5728\xFF1A\n{path}"},

    // --- Tab group section ---
    {"tab.add", L"en", L"+ Add Tab"},
    {"tab.add", L"ja", L"+ \x30BF\x30D6\x8FFD\x52A0"},
    {"tab.add", L"ko", L"+ \xD0ED \xCD94\xAC00"},
    {"tab.add", L"zh_TW", L"+ \x65B0\x589E\x5206\x9801"},
    {"tab.add", L"zh_CN", L"+ \x6DFB\x52A0\x6807\x7B7E"},

    {"tab.delete", L"en", L"x Delete Tab"},
    {"tab.delete", L"ja", L"x \x30BF\x30D6\x524A\x9664"},
    {"tab.delete", L"ko", L"x \xD0ED \xC0AD\xC81C"},
    {"tab.delete", L"zh_TW", L"x \x522A\x9664\x5206\x9801"},
    {"tab.delete", L"zh_CN", L"x \x5220\x9664\x6807\x7B7E"},

    {"tab.rename", L"en", L"Rename"},
    {"tab.rename", L"ja", L"\x540D\x524D\x5909\x66F4"},
    {"tab.rename", L"ko", L"\xC774\xB984 \xBCC0\xACBD"},
    {"tab.rename", L"zh_TW", L"\x91CD\x65B0\x547D\x540D"},
    {"tab.rename", L"zh_CN", L"\x91CD\x547D\x540D"},

    {"tab.copy", L"en", L"Copy Tab"},
    {"tab.copy", L"ja", L"\x30BF\x30D6\x8907\x88FD"},
    {"tab.copy", L"ko", L"\xD0ED \xBCF5\xC0AC"},
    {"tab.copy", L"zh_TW", L"\x8907\x88FD\x5206\x9801"},
    {"tab.copy", L"zh_CN", L"\x590D\x5236\x6807\x7B7E"},

    {"tab.move_left", L"en", L"\x25B2"},
    {"tab.move_left", L"ja", L"\x25B2"},
    {"tab.move_left", L"ko", L"\x25B2"},
    {"tab.move_left", L"zh_TW", L"\x25B2"},
    {"tab.move_left", L"zh_CN", L"\x25B2"},
    {"tab.move_right", L"en", L"\x25BC"},
    {"tab.move_right", L"ja", L"\x25BC"},
    {"tab.move_right", L"ko", L"\x25BC"},
    {"tab.move_right", L"zh_TW", L"\x25BC"},
    {"tab.move_right", L"zh_CN", L"\x25BC"},

    {"tab.add_dialog_title", L"en", L"Add Tab"},
    {"tab.add_dialog_title", L"ja", L"\x30BF\x30D6\x8FFD\x52A0"},
    {"tab.add_dialog_title", L"ko", L"\xD0ED \xCD94\xAC00"},
    {"tab.add_dialog_title", L"zh_TW", L"\x65B0\x589E\x5206\x9801"},
    {"tab.add_dialog_title", L"zh_CN", L"\x6DFB\x52A0\x6807\x7B7E"},

    {"tab.add_dialog_prompt", L"en", L"Enter tab name:"},
    {"tab.add_dialog_prompt", L"ja", L"\x30BF\x30D6\x540D\x3092\x5165\x529B:"},
    {"tab.add_dialog_prompt", L"ko", L"\xD0ED \xC774\xB984 \xC785\xB825:"},
    {"tab.add_dialog_prompt", L"zh_TW", L"\x8F38\x5165\x5206\x9801\x540D\x7A31\xFF1A"},
    {"tab.add_dialog_prompt", L"zh_CN", L"\x8F93\x5165\x6807\x7B7E\x540D\x79F0\xFF1A"},

    {"tab.duplicate_title", L"en", L"Duplicate"},
    {"tab.duplicate_title", L"ja", L"\x91CD\x8907"},
    {"tab.duplicate_title", L"ko", L"\xC911\xBCF5"},
    {"tab.duplicate_title", L"zh_TW", L"\x91CD\x8907"},
    {"tab.duplicate_title", L"zh_CN", L"\x91CD\x590D"},

    {"tab.duplicate_msg", L"en", L"Tab '{name}' already exists."},
    {"tab.duplicate_msg", L"ja", L"\x30BF\x30D6 '{name}' \x306F\x65E2\x306B\x5B58\x5728\x3057\x307E\x3059\x3002"},
    {"tab.duplicate_msg", L"ko", L"\xD0ED '{name}'\xC774(\xAC00) \xC774\xBBF8 \xC874\xC7AC\xD569\xB2C8\xB2E4."},
    {"tab.duplicate_msg", L"zh_TW", L"\x5206\x9801 '{name}' \x5DF2\x5B58\x5728\x3002"},
    {"tab.duplicate_msg", L"zh_CN", L"\x6807\x7B7E '{name}' \x5DF2\x5B58\x5728\x3002"},

    {"tab.rename_dialog_title", L"en", L"Rename Tab"},
    {"tab.rename_dialog_title", L"ja", L"\x30BF\x30D6\x540D\x5909\x66F4"},
    {"tab.rename_dialog_title", L"ko", L"\xD0ED \xC774\xB984 \xBCC0\xACBD"},
    {"tab.rename_dialog_title", L"zh_TW", L"\x91CD\x65B0\x547D\x540D\x5206\x9801"},
    {"tab.rename_dialog_title", L"zh_CN", L"\x91CD\x547D\x540D\x6807\x7B7E"},

    {"tab.rename_dialog_prompt", L"en", L"Enter new name:"},
    {"tab.rename_dialog_prompt", L"ja", L"\x65B0\x3057\x3044\x30BF\x30D6\x540D\x3092\x5165\x529B:"},
    {"tab.rename_dialog_prompt", L"ko", L"\xC0C8 \xC774\xB984 \xC785\xB825:"},
    {"tab.rename_dialog_prompt", L"zh_TW", L"\x8F38\x5165\x65B0\x540D\x7A31\xFF1A"},
    {"tab.rename_dialog_prompt", L"zh_CN", L"\x8F93\x5165\x65B0\x540D\x79F0\xFF1A"},

    {"tab.delete_confirm_title", L"en", L"Delete Tab"},
    {"tab.delete_confirm_title", L"ja", L"\x30BF\x30D6\x524A\x9664"},
    {"tab.delete_confirm_title", L"ko", L"\xD0ED \xC0AD\xC81C"},
    {"tab.delete_confirm_title", L"zh_TW", L"\x522A\x9664\x5206\x9801"},
    {"tab.delete_confirm_title", L"zh_CN", L"\x5220\x9664\x6807\x7B7E"},

    {"tab.delete_confirm_msg", L"en", L"Delete tab '{name}' and all its paths?"},
    {"tab.delete_confirm_msg", L"ja", L"\x30BF\x30D6 '{name}' \x3068\x305D\x306E\x30D1\x30B9\x3092\x3059\x3079\x3066\x524A\x9664\x3057\x307E\x3059\x304B\xFF1F"},
    {"tab.delete_confirm_msg", L"ko", L"\xD0ED '{name}'\xACFC(\xC640) \xBAA8\xB4E0 \xACBD\xB85C\xB97C \xC0AD\xC81C\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?"},
    {"tab.delete_confirm_msg", L"zh_TW", L"\x662F\x5426\x522A\x9664\x5206\x9801 '{name}' \x53CA\x5176\x6240\x6709\x8DEF\x5F91\xFF1F"},
    {"tab.delete_confirm_msg", L"zh_CN", L"\x662F\x5426\x5220\x9664\x6807\x7B7E '{name}' \x53CA\x5176\x6240\x6709\x8DEF\x5F84\xFF1F"},

    {"tab.no_tab_title", L"en", L"No Tab"},
    {"tab.no_tab_title", L"ja", L"\x30BF\x30D6\x306A\x3057"},
    {"tab.no_tab_title", L"ko", L"\xD0ED \xC5C6\xC74C"},
    {"tab.no_tab_title", L"zh_TW", L"\x7121\x5206\x9801"},
    {"tab.no_tab_title", L"zh_CN", L"\x65E0\x6807\x7B7E"},
    {"tab.no_tab_msg", L"en", L"Please add a tab first."},
    {"tab.no_tab_msg", L"ja", L"\x5148\x306B\x30BF\x30D6\x3092\x8FFD\x52A0\x3057\x3066\x304F\x3060\x3055\x3044\x3002"},
    {"tab.no_tab_msg", L"ko", L"\xBA3C\xC800 \xD0ED\xC744 \xCD94\xAC00\xD574 \xC8FC\xC138\xC694."},
    {"tab.no_tab_msg", L"zh_TW", L"\x8ACB\x5148\x65B0\x589E\x5206\x9801\x3002"},
    {"tab.no_tab_msg", L"zh_CN", L"\x8BF7\x5148\x6DFB\x52A0\x6807\x7B7E\x3002"},

    {"tab.no_paths_title", L"en", L"No Paths"},
    {"tab.no_paths_title", L"ja", L"\x30D1\x30B9\x306A\x3057"},
    {"tab.no_paths_title", L"ko", L"\xACBD\xB85C \xC5C6\xC74C"},
    {"tab.no_paths_title", L"zh_TW", L"\x7121\x8DEF\x5F91"},
    {"tab.no_paths_title", L"zh_CN", L"\x65E0\x8DEF\x5F84"},
    {"tab.no_paths_msg", L"en", L"No folders registered in this tab."},
    {"tab.no_paths_msg", L"ja", L"\x3053\x306E\x30BF\x30D6\x306B\x30D5\x30A9\x30EB\x30C0\x304C\x767B\x9332\x3055\x308C\x3066\x3044\x307E\x305B\x3093\x3002"},
    {"tab.no_paths_msg", L"ko", L"\xC774 \xD0ED\xC5D0 \xB4F1\xB85D\xB41C \xD3F4\xB354\xAC00 \xC5C6\xC2B5\xB2C8\xB2E4."},
    {"tab.no_paths_msg", L"zh_TW", L"\x6B64\x5206\x9801\x4E2D\x6C92\x6709\x8A3B\x518A\x7684\x8CC7\x6599\x593E\x3002"},
    {"tab.no_paths_msg", L"zh_CN", L"\x6B64\x6807\x7B7E\x4E2D\x6CA1\x6709\x6CE8\x518C\x7684\x6587\x4EF6\x5939\x3002"},

    {"tab.open_as_tabs", L"en", L"Open as Tabs"},
    {"tab.open_as_tabs", L"ja", L"\x30BF\x30D6\x3067\x958B\x304F"},
    {"tab.open_as_tabs", L"ko", L"\xD0ED\xC73C\xB85C \xC5F4\xAE30"},
    {"tab.open_as_tabs", L"zh_TW", L"\x4EE5\x5206\x9801\x958B\x555F"},
    {"tab.open_as_tabs", L"zh_CN", L"\x4EE5\x6807\x7B7E\x6253\x5F00"},

    // --- Path operations ---
    {"path.move_up", L"en", L"\x25B2 Up"},
    {"path.move_up", L"ja", L"\x25B2 \x4E0A\x3078"},
    {"path.move_up", L"ko", L"\x25B2 \xC704\xB85C"},
    {"path.move_up", L"zh_TW", L"\x25B2 \x4E0A\x79FB"},
    {"path.move_up", L"zh_CN", L"\x25B2 \x4E0A\x79FB"},
    {"path.move_down", L"en", L"\x25BC Down"},
    {"path.move_down", L"ja", L"\x25BC \x4E0B\x3078"},
    {"path.move_down", L"ko", L"\x25BC \xC544\xB798\xB85C"},
    {"path.move_down", L"zh_TW", L"\x25BC \x4E0B\x79FB"},
    {"path.move_down", L"zh_CN", L"\x25BC \x4E0B\x79FB"},

    {"path.add", L"en", L"+ Add Path"},
    {"path.add", L"ja", L"+ \x30D1\x30B9\x8FFD\x52A0"},
    {"path.add", L"ko", L"+ \xACBD\xB85C \xCD94\xAC00"},
    {"path.add", L"zh_TW", L"+ \x65B0\x589E\x8DEF\x5F91"},
    {"path.add", L"zh_CN", L"+ \x6DFB\x52A0\x8DEF\x5F84"},

    {"path.remove", L"en", L"- Remove"},
    {"path.remove", L"ja", L"- \x30D1\x30B9\x524A\x9664"},
    {"path.remove", L"ko", L"- \xACBD\xB85C \xC0AD\xC81C"},
    {"path.remove", L"zh_TW", L"- \x79FB\x9664\x8DEF\x5F91"},
    {"path.remove", L"zh_CN", L"- \x5220\x9664\x8DEF\x5F84"},

    {"path.browse", L"en", L"Browse..."},
    {"path.browse", L"ja", L"\x53C2\x7167..."},
    {"path.browse", L"ko", L"\xCC3E\xC544\xBCF4\xAE30..."},
    {"path.browse", L"zh_TW", L"\x700F\x89BD..."},
    {"path.browse", L"zh_CN", L"\x6D4F\x89C8..."},

    {"path.placeholder", L"en", L"Enter folder path..."},
    {"path.placeholder", L"ja", L"\x30D5\x30A9\x30EB\x30C0\x30D1\x30B9\x3092\x5165\x529B..."},
    {"path.placeholder", L"ko", L"\xD3F4\xB354 \xACBD\xB85C \xC785\xB825..."},
    {"path.placeholder", L"zh_TW", L"\x8F38\x5165\x8CC7\x6599\x593E\x8DEF\x5F91..."},
    {"path.placeholder", L"zh_CN", L"\x8F93\x5165\x6587\x4EF6\x5939\x8DEF\x5F84..."},

    {"path.invalid_title", L"en", L"Invalid Path"},
    {"path.invalid_title", L"ja", L"\x7121\x52B9\x306A\x30D1\x30B9"},
    {"path.invalid_title", L"ko", L"\xC798\xBABB\xB41C \xACBD\xB85C"},
    {"path.invalid_title", L"zh_TW", L"\x7121\x6548\x8DEF\x5F91"},
    {"path.invalid_title", L"zh_CN", L"\x65E0\x6548\x8DEF\x5F84"},
    {"path.invalid_msg", L"en", L"Folder does not exist:\n{path}"},
    {"path.invalid_msg", L"ja", L"\x30D5\x30A9\x30EB\x30C0\x304C\x5B58\x5728\x3057\x307E\x305B\x3093:\n{path}"},
    {"path.invalid_msg", L"ko", L"\xD3F4\xB354\xAC00 \xC874\xC7AC\xD558\xC9C0 \xC54A\xC2B5\xB2C8\xB2E4:\n{path}"},
    {"path.invalid_msg", L"zh_TW", L"\x8CC7\x6599\x593E\x4E0D\x5B58\x5728\xFF1A\n{path}"},
    {"path.invalid_msg", L"zh_CN", L"\x6587\x4EF6\x5939\x4E0D\x5B58\x5728\xFF1A\n{path}"},

    // --- Error messages ---
    {"error.title", L"en", L"Error"},
    {"error.title", L"ja", L"\x30A8\x30E9\x30FC"},
    {"error.title", L"ko", L"\xC624\xB958"},
    {"error.title", L"zh_TW", L"\x932F\x8AA4"},
    {"error.title", L"zh_CN", L"\x9519\x8BEF"},
    {"error.open_failed", L"en", L"Failed to open:\n{path}\n\n{error}"},
    {"error.open_failed", L"ja", L"\x958B\x3051\x307E\x305B\x3093\x3067\x3057\x305F:\n{path}\n\n{error}"},
    {"error.open_failed", L"ko", L"\xC5F4\xAE30 \xC2E4\xD328:\n{path}\n\n{error}"},
    {"error.open_failed", L"zh_TW", L"\x7121\x6CD5\x958B\x555F\xFF1A\n{path}\n\n{error}"},
    {"error.open_failed", L"zh_CN", L"\x65E0\x6CD5\x6253\x5F00\xFF1A\n{path}\n\n{error}"},

    {"error.invalid_paths_title", L"en", L"Invalid Paths"},
    {"error.invalid_paths_title", L"ja", L"\x7121\x52B9\x306A\x30D1\x30B9"},
    {"error.invalid_paths_title", L"ko", L"\xC798\xBABB\xB41C \xACBD\xB85C"},
    {"error.invalid_paths_title", L"zh_TW", L"\x7121\x6548\x8DEF\x5F91"},
    {"error.invalid_paths_title", L"zh_CN", L"\x65E0\x6548\x8DEF\x5F84"},
    {"error.invalid_paths_msg", L"en", L"The following paths will be skipped:\n{paths}"},
    {"error.invalid_paths_msg", L"ja", L"\x4EE5\x4E0B\x306E\x30D1\x30B9\x306F\x30B9\x30AD\x30C3\x30D7\x3055\x308C\x307E\x3059:\n{paths}"},
    {"error.invalid_paths_msg", L"ko", L"\xB2E4\xC74C \xACBD\xB85C\xB294 \xAC74\xB108\xB701\xB2C8\xB2E4:\n{paths}"},
    {"error.invalid_paths_msg", L"zh_TW", L"\x4EE5\x4E0B\x8DEF\x5F91\x5C07\x88AB\x8DF3\x904E\xFF1A\n{paths}"},
    {"error.invalid_paths_msg", L"zh_CN", L"\x4EE5\x4E0B\x8DEF\x5F84\x5C06\x88AB\x8DF3\x8FC7\xFF1A\n{paths}"},

    // --- Toast ---
    {"toast.progress_header", L"en", L"Opening tabs... ({current}/{total})"},
    {"toast.progress_header", L"ja", L"\x30BF\x30D6\x3092\x5C55\x958B\x4E2D... ({current}/{total})"},
    {"toast.progress_header", L"ko", L"\xD0ED \xC5EC\xB294 \xC911... ({current}/{total})"},
    {"toast.progress_header", L"zh_TW", L"\x6B63\x5728\x958B\x555F\x5206\x9801... ({current}/{total})"},
    {"toast.progress_header", L"zh_CN", L"\x6B63\x5728\x6253\x5F00\x6807\x7B7E... ({current}/{total})"},

    {"toast.wait_message", L"en", L"Please wait.\nDo not use the keyboard or mouse."},
    {"toast.wait_message", L"ja", L"\x3057\x3070\x3089\x304F\x304A\x5F85\x3061\x304F\x3060\x3055\x3044\x3002\n\x30AD\x30FC\x30DC\x30FC\x30C9\x30FB\x30DE\x30A6\x30B9\x3092\x64CD\x4F5C\x3057\x306A\x3044\x3067\x304F\x3060\x3055\x3044"},
    {"toast.wait_message", L"ko", L"\xC7A0\xC2DC\xB9CC \xAE30\xB2E4\xB824 \xC8FC\xC138\xC694.\n\xD0A4\xBCF4\xB4DC\xB098 \xB9C8\xC6B0\xC2A4\xB97C \xC0AC\xC6A9\xD558\xC9C0 \xB9C8\xC138\xC694."},
    {"toast.wait_message", L"zh_TW", L"\x8ACB\x7A0D\x5019\x3002\n\x8ACB\x52FF\x4F7F\x7528\x9375\x76E4\x6216\x6ED1\x9F20\x3002"},
    {"toast.wait_message", L"zh_CN", L"\x8BF7\x7A0D\x5019\x3002\n\x8BF7\x52FF\x4F7F\x7528\x952E\x76D8\x6216\x9F20\x6807\x3002"},

    // --- Window geometry ---
    {"window.x", L"en", L"X:"},
    {"window.x", L"ja", L"X:"},
    {"window.x", L"ko", L"X:"},
    {"window.x", L"zh_TW", L"X:"},
    {"window.x", L"zh_CN", L"X:"},
    {"window.y", L"en", L"Y:"},
    {"window.y", L"ja", L"Y:"},
    {"window.y", L"ko", L"Y:"},
    {"window.y", L"zh_TW", L"Y:"},
    {"window.y", L"zh_CN", L"Y:"},
    {"window.width", L"en", L"Width:"},
    {"window.width", L"ja", L"\x5E45:"},
    {"window.width", L"ko", L"\xB108\xBE44:"},
    {"window.width", L"zh_TW", L"\x5BEC\x5EA6:"},
    {"window.width", L"zh_CN", L"\x5BBD\x5EA6:"},
    {"window.height", L"en", L"Height:"},
    {"window.height", L"ja", L"\x9AD8\x3055:"},
    {"window.height", L"ko", L"\xB192\xC774:"},
    {"window.height", L"zh_TW", L"\x9AD8\x5EA6:"},
    {"window.height", L"zh_CN", L"\x9AD8\x5EA6:"},

    {"window.get_from_explorer", L"en", L"Get from Explorer"},
    {"window.get_from_explorer", L"ja", L"Explorer\x304B\x3089\x53D6\x5F97"},
    {"window.get_from_explorer", L"ko", L"\xD0D0\xC0C9\xAE30\xC5D0\xC11C \xAC00\xC838\xC624\xAE30"},
    {"window.get_from_explorer", L"zh_TW", L"\x5F9E\x6A94\x6848\x7E3D\x7BA1\x53D6\x5F97"},
    {"window.get_from_explorer", L"zh_CN", L"\x4ECE\x8D44\x6E90\x7BA1\x7406\x5668\x83B7\x53D6"},
    {"window.no_explorer_title", L"en", L"No Explorer Window"},
    {"window.no_explorer_title", L"ja", L"Explorer\x30A6\x30A3\x30F3\x30C9\x30A6\x306A\x3057"},
    {"window.no_explorer_title", L"ko", L"\xD0D0\xC0C9\xAE30 \xCC3D \xC5C6\xC74C"},
    {"window.no_explorer_title", L"zh_TW", L"\x7121\x6A94\x6848\x7E3D\x7BA1\x8996\x7A97"},
    {"window.no_explorer_title", L"zh_CN", L"\x65E0\x8D44\x6E90\x7BA1\x7406\x5668\x7A97\x53E3"},
    {"window.no_explorer_msg", L"en", L"No Explorer window is open."},
    {"window.no_explorer_msg", L"ja", L"Explorer\x30A6\x30A3\x30F3\x30C9\x30A6\x304C\x958B\x3044\x3066\x3044\x307E\x305B\x3093\x3002"},
    {"window.no_explorer_msg", L"ko", L"\xC5F4\xB9B0 \xD0D0\xC0C9\xAE30 \xCC3D\xC774 \xC5C6\xC2B5\xB2C8\xB2E4."},
    {"window.no_explorer_msg", L"zh_TW", L"\x6C92\x6709\x958B\x555F\x7684\x6A94\x6848\x7E3D\x7BA1\x8996\x7A97\x3002"},
    {"window.no_explorer_msg", L"zh_CN", L"\x6CA1\x6709\x6253\x5F00\x7684\x8D44\x6E90\x7BA1\x7406\x5668\x7A97\x53E3\x3002"},

    // --- Settings ---
    {"settings.timeout", L"en", L"Timeout"},
    {"settings.timeout", L"ja", L"\x30BF\x30A4\x30E0\x30A2\x30A6\x30C8"},
    {"settings.timeout", L"ko", L"\xC2DC\xAC04 \xCD08\xACFC"},
    {"settings.timeout", L"zh_TW", L"\x903E\x6642"},
    {"settings.timeout", L"zh_CN", L"\x8D85\x65F6"},
    {"settings.timeout_unit", L"en", L"sec"},
    {"settings.timeout_unit", L"ja", L"\x79D2"},
    {"settings.timeout_unit", L"ko", L"\xCD08"},
    {"settings.timeout_unit", L"zh_TW", L"\x79D2"},
    {"settings.timeout_unit", L"zh_CN", L"\x79D2"},
};
// clang-format on

// Build lookup table on first use
using TransMap = std::unordered_map<std::string,
    std::unordered_map<std::wstring, std::wstring>>;

static const TransMap& get_trans_map() {
    static TransMap map = []() {
        TransMap m;
        for (const auto& e : g_strings) {
            m[e.key][e.lang] = e.text;
        }
        return m;
    }();
    return map;
}

void i18n_init() {
    auto lang = detect_system_language();
    set_language(lang);
}

void set_language(const std::wstring& lang) {
    for (const auto& li : supported_langs()) {
        if (lang == li.code) {
            g_current_lang = lang;
            LOG_INFO("i18n", "Language set to: %s", wide_to_utf8(lang).c_str());
            return;
        }
    }
    LOG_WARN("i18n", "Unsupported language: %s, falling back to en",
             wide_to_utf8(lang).c_str());
    g_current_lang = L"en";
}

std::wstring get_language() {
    return g_current_lang;
}

std::wstring detect_system_language() {
    wchar_t buf[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH) > 0) {
        std::wstring locale(buf);
        // Convert to lowercase for matching
        std::transform(locale.begin(), locale.end(), locale.begin(), ::towlower);
        if (locale.substr(0, 2) == L"ja") return L"ja";
        if (locale.substr(0, 2) == L"ko") return L"ko";
        if (locale.substr(0, 2) == L"zh") {
            if (locale.find(L"tw") != std::wstring::npos ||
                locale.find(L"hk") != std::wstring::npos ||
                locale.find(L"hant") != std::wstring::npos) {
                return L"zh_TW";
            }
            return L"zh_CN";
        }
    }
    return L"en";
}

std::wstring t(const std::string& key) {
    const auto& map = get_trans_map();
    auto it = map.find(key);
    if (it == map.end()) {
        LOG_WARN("i18n", "Missing key: %s", key.c_str());
        return utf8_to_wide(key);
    }
    const auto& langs = it->second;
    auto lit = langs.find(g_current_lang);
    if (lit != langs.end()) return lit->second;
    // Fallback to English
    lit = langs.find(L"en");
    if (lit != langs.end()) return lit->second;
    return utf8_to_wide(key);
}

std::wstring t(const std::string& key,
               const std::unordered_map<std::string, std::wstring>& kwargs) {
    std::wstring text = t(key);
    for (const auto& [k, v] : kwargs) {
        std::wstring placeholder = L"{" + utf8_to_wide(k) + L"}";
        size_t pos = 0;
        while ((pos = text.find(placeholder, pos)) != std::wstring::npos) {
            text.replace(pos, placeholder.size(), v);
            pos += v.size();
        }
    }
    return text;
}

} // namespace fto

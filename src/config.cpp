#include "config.h"
#include "utils.h"
#include "logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <regex>

using json = nlohmann::json;

namespace fto {

void HistoryEntry::touch() {
    last_used = get_iso_timestamp();
    use_count++;
}

ConfigManager::ConfigManager() {
    path_ = get_appdata_dir() + L"\\config.json";
}

void ConfigManager::load() {
    std::string u8path = wide_to_utf8(path_);
    if (!std::filesystem::exists(u8path)) {
        LOG_DEBUG("config", "Config not found, using defaults: %s", u8path.c_str());
        return;
    }
    try {
        std::ifstream f(path_);
        json j = json::parse(f);

        data_ = AppConfig{};

        // History
        if (j.contains("history") && j["history"].is_array()) {
            for (const auto& item : j["history"]) {
                HistoryEntry e;
                e.path = utf8_to_wide(item.value("path", ""));
                e.pinned = item.value("pinned", false);
                e.last_used = item.value("last_used", "");
                e.use_count = item.value("use_count", 0);
                data_.history.push_back(std::move(e));
            }
        }

        // Tab groups
        if (j.contains("tab_groups") && j["tab_groups"].is_array()) {
            for (const auto& item : j["tab_groups"]) {
                TabGroup g;
                g.name = utf8_to_wide(item.value("name", ""));
                auto paths_key = item.contains("paths") ? "paths" : "folders";
                if (item.contains(paths_key) && item[paths_key].is_array()) {
                    for (const auto& p : item[paths_key]) {
                        g.paths.push_back(utf8_to_wide(p.get<std::string>()));
                    }
                }
                if (item.contains("window_x") && !item["window_x"].is_null())
                    g.window_x = item["window_x"].get<int>();
                if (item.contains("window_y") && !item["window_y"].is_null())
                    g.window_y = item["window_y"].get<int>();
                if (item.contains("window_width") && !item["window_width"].is_null())
                    g.window_width = item["window_width"].get<int>();
                if (item.contains("window_height") && !item["window_height"].is_null())
                    g.window_height = item["window_height"].get<int>();
                data_.tab_groups.push_back(std::move(g));
            }
        }

        data_.window_geometry = j.value("window_geometry", "800x600");

        if (j.contains("settings") && j["settings"].is_object()) {
            for (auto& [k, v] : j["settings"].items()) {
                if (v.is_string()) data_.settings[k] = v.get<std::string>();
                else if (v.is_number_integer()) data_.settings[k] = std::to_string(v.get<int>());
                else if (v.is_boolean()) data_.settings[k] = v.get<bool>() ? "true" : "false";
            }
        }

        LOG_DEBUG("config", "Loaded: %d history, %d tab groups",
                  (int)data_.history.size(), (int)data_.tab_groups.size());
    } catch (const std::exception& e) {
        LOG_WARN("config", "Config corrupt, using defaults: %s", e.what());
        data_ = AppConfig{};
    }
}

void ConfigManager::save() {
    std::filesystem::create_directories(wide_to_utf8(get_appdata_dir()));

    json j;

    // History
    json hist = json::array();
    for (const auto& e : data_.history) {
        hist.push_back({
            {"path", wide_to_utf8(e.path)},
            {"pinned", e.pinned},
            {"last_used", e.last_used},
            {"use_count", e.use_count},
        });
    }
    j["history"] = hist;

    // Tab groups
    json groups = json::array();
    for (const auto& g : data_.tab_groups) {
        json paths = json::array();
        for (const auto& p : g.paths) paths.push_back(wide_to_utf8(p));
        json gj = {
            {"name", wide_to_utf8(g.name)},
            {"paths", paths},
            {"window_x", g.window_x.has_value() ? json(g.window_x.value()) : json(nullptr)},
            {"window_y", g.window_y.has_value() ? json(g.window_y.value()) : json(nullptr)},
            {"window_width", g.window_width.has_value() ? json(g.window_width.value()) : json(nullptr)},
            {"window_height", g.window_height.has_value() ? json(g.window_height.value()) : json(nullptr)},
        };
        groups.push_back(gj);
    }
    j["tab_groups"] = groups;
    j["window_geometry"] = data_.window_geometry;

    // Settings
    json settings;
    for (const auto& [k, v] : data_.settings) {
        // Try to preserve numeric/bool types for Python compat
        if (v == "true") settings[k] = true;
        else if (v == "false") settings[k] = false;
        else {
            try { settings[k] = std::stoi(v); }
            catch (...) { settings[k] = v; }
        }
    }
    j["settings"] = settings;

    // Atomic write: write to tmp file, then rename
    std::wstring tmp_path = path_ + L".tmp";
    {
        std::ofstream f(tmp_path);
        if (!f.is_open()) {
            LOG_WARN("config", "Failed to open tmp file for writing");
            return;
        }
        f << j.dump(2, ' ', false, json::error_handler_t::replace);
        if (f.fail()) {
            LOG_WARN("config", "Failed to write config to tmp file");
            return;
        }
    }
    std::error_code ec;
    std::filesystem::rename(wide_to_utf8(tmp_path), wide_to_utf8(path_), ec);
    if (ec) {
        // Fallback: direct write
        LOG_WARN("config", "Rename failed (%s), falling back to direct write", ec.message().c_str());
        std::ofstream f(path_);
        f << j.dump(2, ' ', false, json::error_handler_t::replace);
    }
    LOG_DEBUG("config", "Saved: %s", wide_to_utf8(path_).c_str());
}

// --- History ---

void ConfigManager::add_history(const std::wstring& path) {
    std::wstring clean = strip_quotes(path);
    std::wstring normalized = normalize_path(clean);
    for (auto& e : data_.history) {
        if (normalize_path(e.path) == normalized) {
            e.touch();
            return;
        }
    }
    HistoryEntry ne;
    ne.path = normalized;
    ne.touch();
    data_.history.push_back(std::move(ne));
    trim_history();
}

void ConfigManager::remove_history(const std::wstring& path) {
    std::wstring normalized = normalize_path(path);
    data_.history.erase(
        std::remove_if(data_.history.begin(), data_.history.end(),
            [&](const HistoryEntry& e) { return normalize_path(e.path) == normalized; }),
        data_.history.end());
}

void ConfigManager::clear_history(bool keep_pinned) {
    if (keep_pinned) {
        data_.history.erase(
            std::remove_if(data_.history.begin(), data_.history.end(),
                [](const HistoryEntry& e) { return !e.pinned; }),
            data_.history.end());
    } else {
        data_.history.clear();
    }
}

void ConfigManager::toggle_pin(const std::wstring& path) {
    std::wstring normalized = normalize_path(path);
    for (auto& e : data_.history) {
        if (normalize_path(e.path) == normalized) {
            e.pinned = !e.pinned;
            return;
        }
    }
}

std::vector<HistoryEntry> ConfigManager::get_sorted_history() const {
    std::vector<HistoryEntry> pinned, unpinned;
    for (const auto& e : data_.history) {
        if (e.pinned) pinned.push_back(e);
        else unpinned.push_back(e);
    }
    auto cmp = [](const HistoryEntry& a, const HistoryEntry& b) {
        return a.last_used > b.last_used;
    };
    std::sort(pinned.begin(), pinned.end(), cmp);
    std::sort(unpinned.begin(), unpinned.end(), cmp);
    pinned.insert(pinned.end(), unpinned.begin(), unpinned.end());
    return pinned;
}

void ConfigManager::trim_history() {
    if ((int)data_.history.size() <= HISTORY_MAX) return;
    std::vector<HistoryEntry> pinned, unpinned;
    for (auto& e : data_.history) {
        if (e.pinned) pinned.push_back(std::move(e));
        else unpinned.push_back(std::move(e));
    }
    int keep = std::max(HISTORY_MAX - (int)pinned.size(), 0);
    std::sort(unpinned.begin(), unpinned.end(),
              [](const HistoryEntry& a, const HistoryEntry& b) {
                  return a.last_used > b.last_used;
              });
    if ((int)unpinned.size() > keep) unpinned.resize(keep);
    data_.history = std::move(pinned);
    data_.history.insert(data_.history.end(), unpinned.begin(), unpinned.end());
}

// --- Tab groups ---

TabGroup* ConfigManager::add_tab_group(const std::wstring& name) {
    if (name.empty()) return nullptr;
    // Check for whitespace-only
    bool all_space = true;
    for (auto c : name) { if (!iswspace(c)) { all_space = false; break; } }
    if (all_space) return nullptr;
    if (get_tab_group(name)) return nullptr;
    data_.tab_groups.push_back(TabGroup{name});
    LOG_INFO("config", "Tab group added: %s", wide_to_utf8(name).c_str());
    return &data_.tab_groups.back();
}

void ConfigManager::delete_tab_group(const std::wstring& name) {
    data_.tab_groups.erase(
        std::remove_if(data_.tab_groups.begin(), data_.tab_groups.end(),
            [&](const TabGroup& g) { return g.name == name; }),
        data_.tab_groups.end());
}

void ConfigManager::rename_tab_group(const std::wstring& old_name, const std::wstring& new_name) {
    for (auto& g : data_.tab_groups) {
        if (g.name == old_name) { g.name = new_name; return; }
    }
}

TabGroup* ConfigManager::get_tab_group(const std::wstring& name) {
    for (auto& g : data_.tab_groups) {
        if (g.name == name) return &g;
    }
    return nullptr;
}

void ConfigManager::add_path_to_group(const std::wstring& group_name, const std::wstring& path) {
    auto* g = get_tab_group(group_name);
    if (g) g->paths.push_back(normalize_path(path));
}

void ConfigManager::remove_path_from_group(const std::wstring& group_name, int index) {
    auto* g = get_tab_group(group_name);
    if (g && index >= 0 && index < (int)g->paths.size()) {
        g->paths.erase(g->paths.begin() + index);
    }
}

void ConfigManager::move_tab_group(int old_index, int new_index) {
    auto& groups = data_.tab_groups;
    if (old_index < 0 || old_index >= (int)groups.size()) return;
    if (new_index < 0 || new_index >= (int)groups.size()) return;
    auto item = std::move(groups[old_index]);
    groups.erase(groups.begin() + old_index);
    groups.insert(groups.begin() + new_index, std::move(item));
}

TabGroup* ConfigManager::copy_tab_group(const std::wstring& name) {
    auto* src = get_tab_group(name);
    if (!src) return nullptr;

    // Extract base name: strip trailing ' <digits>'
    std::wstring base = name;
    std::wregex re(LR"(^(.*?)\s+(\d+)$)");
    std::wsmatch match;
    if (std::regex_match(name, match, re)) {
        base = match[1].str();
    }

    std::unordered_map<std::wstring, bool> existing;
    for (const auto& g : data_.tab_groups) existing[g.name] = true;

    int suffix = 1;
    while (existing.count(base + L" " + std::to_wstring(suffix))) suffix++;
    std::wstring new_name = base + L" " + std::to_wstring(suffix);

    TabGroup ng;
    ng.name = new_name;
    ng.paths = src->paths;
    ng.window_x = src->window_x;
    ng.window_y = src->window_y;
    ng.window_width = src->window_width;
    ng.window_height = src->window_height;
    data_.tab_groups.push_back(std::move(ng));
    LOG_INFO("config", "Tab copied: %s -> %s", wide_to_utf8(name).c_str(),
             wide_to_utf8(new_name).c_str());
    return &data_.tab_groups.back();
}

void ConfigManager::move_path_in_group(const std::wstring& group_name, int old_index, int new_index) {
    auto* g = get_tab_group(group_name);
    if (!g) return;
    if (old_index < 0 || old_index >= (int)g->paths.size()) return;
    if (new_index < 0 || new_index >= (int)g->paths.size()) return;
    auto item = std::move(g->paths[old_index]);
    g->paths.erase(g->paths.begin() + old_index);
    g->paths.insert(g->paths.begin() + new_index, std::move(item));
}

int ConfigManager::get_timeout() const {
    auto it = data_.settings.find("timeout");
    if (it != data_.settings.end()) {
        try { return std::stoi(it->second); }
        catch (...) {}
    }
    return 30;
}

std::string ConfigManager::get_setting(const std::string& key, const std::string& default_val) const {
    auto it = data_.settings.find(key);
    return (it != data_.settings.end()) ? it->second : default_val;
}

void ConfigManager::set_setting(const std::string& key, const std::string& value) {
    data_.settings[key] = value;
}

} // namespace fto

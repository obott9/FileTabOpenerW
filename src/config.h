#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace fto {

struct HistoryEntry {
    std::wstring path;
    bool pinned = false;
    std::string last_used;
    int use_count = 0;

    void touch();
};

struct TabGroup {
    std::wstring name;
    std::vector<std::wstring> paths;
    std::optional<int> window_x;
    std::optional<int> window_y;
    std::optional<int> window_width;
    std::optional<int> window_height;
};

struct AppConfig {
    int config_version = 1;
    std::vector<HistoryEntry> history;
    std::vector<TabGroup> tab_groups;
    std::string window_geometry = "800x600";
    std::unordered_map<std::string, nlohmann::json> settings;

    AppConfig() = default;
};

class ConfigManager {
public:
    ConfigManager();

    void load();
    void save();

    const std::wstring& path() const { return path_; }
    AppConfig& data() { return data_; }

    // History operations
    void add_history(const std::wstring& path);
    void remove_history(const std::wstring& path);
    void clear_history(bool keep_pinned = true);
    void toggle_pin(const std::wstring& path);
    std::vector<HistoryEntry> get_sorted_history() const;

    // Tab group operations
    TabGroup* add_tab_group(const std::wstring& name);
    void delete_tab_group(const std::wstring& name);
    void rename_tab_group(const std::wstring& old_name, const std::wstring& new_name);
    TabGroup* get_tab_group(const std::wstring& name);
    void add_path_to_group(const std::wstring& group_name, const std::wstring& path);
    void remove_path_from_group(const std::wstring& group_name, int index);
    void move_tab_group(int old_index, int new_index);
    TabGroup* copy_tab_group(const std::wstring& name);
    void move_path_in_group(const std::wstring& group_name, int old_index, int new_index);

    // Settings helpers
    int get_timeout() const;
    std::string get_setting(const std::string& key, const std::string& default_val = "") const;
    void set_setting(const std::string& key, const std::string& value);

private:
    void trim_history();
    std::wstring path_;
    AppConfig data_;
    static constexpr int HISTORY_MAX = 50;
};

} // namespace fto

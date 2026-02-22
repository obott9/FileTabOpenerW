#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace fto {

enum class LogLevel { Debug, Info, Warning, Error };

class Logger {
public:
    static Logger& instance();

    void init(const std::wstring& log_dir);
    void log(LogLevel level, const char* module, const char* fmt, ...);
    void log(LogLevel level, const char* module, const wchar_t* fmt, ...);

private:
    Logger() = default;
    void rotate_if_needed();
    void write_line(LogLevel level, const char* module, const std::string& message);

    std::ofstream file_;
    std::wstring log_path_;
    std::mutex mutex_;
    bool initialized_ = false;
    static constexpr size_t MAX_FILE_SIZE = 1'000'000; // 1MB
    static constexpr int MAX_BACKUPS = 3;
};

} // namespace fto

// Convenience macros
#define LOG_DEBUG(module, ...) fto::Logger::instance().log(fto::LogLevel::Debug, module, __VA_ARGS__)
#define LOG_INFO(module, ...)  fto::Logger::instance().log(fto::LogLevel::Info, module, __VA_ARGS__)
#define LOG_WARN(module, ...)  fto::Logger::instance().log(fto::LogLevel::Warning, module, __VA_ARGS__)
#define LOG_ERROR(module, ...) fto::Logger::instance().log(fto::LogLevel::Error, module, __VA_ARGS__)

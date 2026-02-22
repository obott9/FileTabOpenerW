#include "logger.h"
#include "utils.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fto {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::init(const std::wstring& log_dir) {
    std::lock_guard lock(mutex_);
    std::filesystem::create_directories(log_dir);
    log_path_ = log_dir + L"\\debug.log";
    file_.open(log_path_, std::ios::app | std::ios::binary);
    initialized_ = file_.is_open();
    if (initialized_) {
        // Also attach a console for INFO output
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stderr);
        }
    }
}

static const char* level_str(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "?";
}

static std::string get_time_str() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_s(&tm_buf, &t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return buf;
}

void Logger::log(LogLevel level, const char* module, const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write_line(level, module, buf);
}

void Logger::log(LogLevel level, const char* module, const wchar_t* fmt, ...) {
    wchar_t wbuf[2048];
    va_list args;
    va_start(args, fmt);
    vswprintf(wbuf, sizeof(wbuf) / sizeof(wchar_t), fmt, args);
    va_end(args);
    write_line(level, module, wide_to_utf8(wbuf));
}

void Logger::write_line(LogLevel level, const char* module, const std::string& message) {
    std::lock_guard lock(mutex_);
    auto time = get_time_str();
    auto lstr = level_str(level);

    // Console: INFO and above
    if (level >= LogLevel::Info) {
        fprintf(stderr, "%s [%s] %s: %s\n", time.c_str(), lstr, module, message.c_str());
    }

    // File: DEBUG and above
    if (initialized_) {
        rotate_if_needed();
        char line[4096];
        snprintf(line, sizeof(line), "%s [%s] %s: %s\n",
                 time.c_str(), lstr, module, message.c_str());
        file_ << line;
        file_.flush();
    }
}

void Logger::rotate_if_needed() {
    if (!initialized_ || log_path_.empty()) return;

    file_.flush();
    auto pos = file_.tellp();
    if (pos < 0 || static_cast<size_t>(pos) < MAX_FILE_SIZE) return;

    file_.close();

    // Rotate: debug.log.3 -> delete, debug.log.2 -> .3, debug.log.1 -> .2, debug.log -> .1
    for (int i = MAX_BACKUPS; i >= 1; --i) {
        auto src = log_path_ + (i > 1 ? L"." + std::to_wstring(i - 1) : L"");
        auto dst = log_path_ + L"." + std::to_wstring(i);
        if (i == 1) src = log_path_;
        DeleteFileW(dst.c_str());
        MoveFileW(src.c_str(), dst.c_str());
    }

    file_.open(log_path_, std::ios::out | std::ios::binary);
    initialized_ = file_.is_open();
}

} // namespace fto

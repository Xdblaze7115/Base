#include <windows.h>
#include <iostream>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include "Logger.hpp"

static void EnableANSIColors() {
    HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out_handle != INVALID_HANDLE_VALUE) {
        DWORD console_mode = 0;
        if (GetConsoleMode(out_handle, &console_mode)) {
            console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(out_handle, console_mode);
        }
    }
}

Logger::Logger() {
    singleton = this;
    EnableANSIColors();
}

Logger* Logger::Singleton() {
    return singleton;
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_s(&tm, &time_t);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
    return std::string(buffer) + "." + std::to_string(ms.count());
}

std::string Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: {
            return "DEBUG";
        }
        case LOG_INFO: {
            return "INFO";
        }
        case LOG_WARN: {
            return "WARN";
        }
        case LOG_ERROR: {
            return "ERROR";
        }
    }
    return "UNKNOWN";
}

std::string Logger::GetLevelColor(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: {
            return "\033[90m";
        }
        case LOG_INFO: {
            return "\033[36m";
        }
        case LOG_WARN: {
            return "\033[33m";
        }
        case LOG_ERROR: {
            return "\033[31m";
        }
    }
    return "\033[0m";
}

std::string Logger::GetResetColor() {
    return "\033[0m";
}

void Logger::Print(LogLevel level, const char* str) {
    std::lock_guard<std::mutex> lock(mutex);
    std::string message = GetLevelColor(level) + "[" + GetTimestamp() + "] [" + GetLevelString(level) + "] " + str + GetResetColor();
    std::cout << message << std::endl;
}

void Logger::Print(LogLevel level, const std::string& str) {
    Print(level, str.c_str());
}

void Logger::Printf(LogLevel level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);
    Print(level, buffer);
}
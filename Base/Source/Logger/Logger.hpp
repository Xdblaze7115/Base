#pragma once
#include <mutex>
#include <string>

enum LogLevel {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
};

class Logger {
private:
	static inline Logger* singleton = nullptr;
	std::mutex mutex;
public:
	Logger();
	static Logger* Singleton();

	void Print(LogLevel level, const char* str);
	void Print(LogLevel level, const std::string& str);
	void Printf(LogLevel level, const char* fmt, ...);
private:
	std::string GetTimestamp();
	std::string GetLevelString(LogLevel level);
	std::string GetLevelColor(LogLevel level);
	std::string GetResetColor();
};
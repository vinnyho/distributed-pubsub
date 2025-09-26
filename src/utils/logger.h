#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3 };

class Logger {
public:
  static LogLevel current_level;

  static void setLevel(LogLevel level) { current_level = level; }

  static void log(LogLevel level, const std::string &component,
                  const std::string &message) {
    if (level < current_level) {
      return; 
    }

    std::string timestamp = getCurrentTime();
    std::string level_str = getLevelString(level);

    std::ostream &output = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
    output << "[" << timestamp << "] " << level_str << " " << component << ": "
           << message << std::endl;
  }

  static void debug(const std::string &component, const std::string &message) {
    log(LogLevel::DEBUG, component, message);
  }

  static void info(const std::string &component, const std::string &message) {
    log(LogLevel::INFO, component, message);
  }

  static void warning(const std::string &component,
                      const std::string &message) {
    log(LogLevel::WARNING, component, message);
  }

  static void error(const std::string &component, const std::string &message) {
    log(LogLevel::ERROR, component, message);
  }

private:
  static std::string getCurrentTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  static std::string getLevelString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:
      return "[DEBUG]";
    case LogLevel::INFO:
      return "[INFO] ";
    case LogLevel::WARNING:
      return "[WARN] ";
    case LogLevel::ERROR:
      return "[ERROR]";
    default:
      return "[UNKNOWN]";
    }
  }
};

LogLevel Logger::current_level = LogLevel::INFO;

#define LOG_DEBUG(component, msg) Logger::debug(component, msg)
#define LOG_INFO(component, msg) Logger::info(component, msg)
#define LOG_WARNING(component, msg) Logger::warning(component, msg)
#define LOG_ERROR(component, msg) Logger::error(component, msg)

#endif 
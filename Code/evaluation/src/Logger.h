#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage): Macro needed for log values
#define LOG(level) \
  if (true) Logger(level, __FILE__, __LINE__, static_cast<const char*>(__FUNCTION__)).stream()

enum LogLevel : char { CRITICAL, ERROR, WARN, INFO, DEBUG, TRACE };

std::string_view toStr(LogLevel level);

class Logger {
 public:
  Logger(LogLevel level, const std::filesystem::path& path, int line, std::string_view function);
  ~Logger();

  // Delete all other constructors & assignment
  Logger(const Logger&)             = delete;
  Logger& operator=(const Logger&)  = delete;
  Logger(const Logger&&)            = delete;
  Logger& operator=(const Logger&&) = delete;

  static void addFile(const std::string& path, LogLevel level = INFO) {
    Workers::addWorker(path, level);
  }

  static void addConsole(LogLevel level = INFO) { Workers::addWorker("", level); }

  static void setConsoleLevel(LogLevel level) { Workers::setLevel("", level); }

  static void setLevel(const std::filesystem::path& path, LogLevel level) {
    Workers::setLevel(path, level);
  }

  static void removeFile(const std::filesystem::path& path) { Workers::removeWorker(path); }

  static void removeConsole() { Workers::removeWorker(""); }

  std::ostream& stream() { return m_buffer; }

 private:
  LogLevel m_level;

  std::ostringstream m_buffer;

  class Workers {
   public:
    static void addWorker(const std::filesystem::path& path, LogLevel level = INFO);

    static void setLevel(const std::filesystem::path& path, LogLevel level);

    static void removeWorker(const std::filesystem::path& path);

    static void log(LogLevel level, std::string_view msg);

   private:
    class Worker {
     public:
      explicit Worker(LogLevel level)
          : m_level(level) {}
      virtual ~Worker() = default;

      // Delete all other constructors & assignment
      Worker(const Worker&)             = delete;
      Worker& operator=(const Worker&)  = delete;
      Worker(const Worker&&)            = delete;
      Worker& operator=(const Worker&&) = delete;

      void log(LogLevel level, std::string_view msg);

      void setLevel(LogLevel level);

     protected:
      void logTime();

      void logOpeningMessage(std::string_view location);

     private:
      virtual std::ostream& stream() = 0;

      virtual std::string_view log_color(LogLevel /*unused*/) { return ""; }
      virtual std::string_view reset_color() { return ""; }

      std::mutex m_mutex;
      LogLevel   m_level;
    };

    class ConsoleWorker : public Worker {
     public:
      explicit ConsoleWorker(LogLevel level)
          : Worker(level) {
        logOpeningMessage("Console");
      }
      // ~ConsoleWorker() override = default;

     private:
      std::ostream& stream() override { return std::cout; }

      std::string_view log_color(LogLevel level) override;
      std::string_view reset_color() override { return "\033[0m"; }
    };

    class FileWorker : public Worker {
     public:
      FileWorker(LogLevel level, const std::filesystem::path& path);
      // ~FileWorker() override = default;

     private:
      std::ostream& stream() override { return m_stream; }

      std::ofstream m_stream;
    };

    static std::shared_mutex& mutex() {
      static std::shared_mutex mutex;
      return mutex;
    }

    static std::map<std::string, std::unique_ptr<Worker>>& workers() {
      static std::map<std::string, std::unique_ptr<Worker>> workers;
      return workers;
    }
  };
};

class ScopedConsoleLogger {
 public:
  ScopedConsoleLogger(LogLevel level = INFO) { Logger::addConsole(level); }
  ~ScopedConsoleLogger() { Logger::removeConsole(); }
};

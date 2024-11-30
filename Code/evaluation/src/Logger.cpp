#include "Logger.h"

#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <string>
#include <string_view>

#include "TimeUtil.h"

static constexpr unsigned int LEVEL_FMT_WIDTH   = 8;
static constexpr unsigned int TIME_FMT_WIDTH    = 25;
static constexpr unsigned int FULL_HEADER_WIDTH = 100;

Logger::Logger(LogLevel level, const std::filesystem::path& path, int line,
               std::string_view function)
    : m_level(level) {
  m_buffer << "[" << std::setw(LEVEL_FMT_WIDTH) << toStr(level) << "] " << path.filename().c_str()
           << ":" << line << " in " << function << "(): ";
}

Logger::~Logger() { Workers::log(m_level, m_buffer.str()); }

void Logger::Workers::addWorker(const std::filesystem::path& path, LogLevel level) {
  const std::lock_guard<std::shared_mutex> lock(mutex());
  if (path == "") {
    workers().try_emplace(path, std::make_unique<ConsoleWorker>(level));
  } else {
    workers().try_emplace(path, std::make_unique<FileWorker>(level, path));
  }
}

void Logger::Workers::setLevel(const std::filesystem::path& path, LogLevel level) {
  const std::lock_guard<std::shared_mutex> lock(mutex());
  workers().at(path)->setLevel(level);
}

void Logger::Workers::removeWorker(const std::filesystem::path& path) {
  const std::lock_guard<std::shared_mutex> lock(mutex());
  workers().erase(path);
}

void Logger::Workers::log(LogLevel level, std::string_view msg) {
  const std::shared_lock<std::shared_mutex> lock(mutex());
  for (auto& [_, worker] : workers()) { worker->log(level, msg); }
}

void Logger::Workers::Worker::log(LogLevel level, std::string_view msg) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (level > m_level) { return; }
  logTime();
  stream() << log_color(level);
  std::size_t start = 0;
  while (start < msg.size()) {
    std::size_t end = msg.find('\n', start);
    if (end == std::string::npos) { end = msg.size(); }
    if (start > 0) { stream() << std::string(TIME_FMT_WIDTH + LEVEL_FMT_WIDTH + 3, ' ') << "-> "; }
    stream() << msg.substr(start, end + 1);
    start = end + 1;
  }
  stream() << reset_color() << '\n';
}

void Logger::Workers::Worker::setLevel(LogLevel level) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  logTime();
  stream() << "Changing log level from " << toStr(m_level) << " to " << toStr(level) << '\n';
  m_level = level;
}

void Logger::Workers::Worker::logTime() { stream() << current_time() << " | "; }

void Logger::Workers::Worker::logOpeningMessage(std::string_view location) {
  stream() << std::string(TIME_FMT_WIDTH, '-') << '\n';
  logTime();
  stream() << std::string(FULL_HEADER_WIDTH - TIME_FMT_WIDTH, '-') << '\n';
  logTime();
  stream() << "Opening \"" << location << "\" for logging at level " << toStr(m_level) << '\n';
}

Logger::Workers::FileWorker::FileWorker(LogLevel level, const std::filesystem::path& path)
    : Worker(level) {
  if (!std::filesystem::exists(path) && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  m_stream.open(path, std::ios::app);
  m_stream << std::unitbuf;
  if (!m_stream.good()) { std::cerr << "Unable to open log at " << path << '\n'; }
  logOpeningMessage(path.string());
}

std::string_view toStr(LogLevel level) {
  switch (level) {
    case CRITICAL: return "CRITICAL";
    case ERROR:    return "ERROR";
    case WARN:     return "WARN";
    case INFO:     return "INFO";
    case DEBUG:    return "DEBUG";
    case TRACE:    return "TRACE";
  }
}

std::string_view Logger::Workers::ConsoleWorker::log_color(LogLevel level) {
  switch (level) {
    case CRITICAL: return "\033[31;1m";
    case ERROR:    return "\033[31m";
    case WARN:     return "\033[33m";
    case INFO:     return "";
    case DEBUG:    return "\033[2m";
    case TRACE:    return "\033[2;3m";
  }
}

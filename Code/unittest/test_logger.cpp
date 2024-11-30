#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>

#include "Logger.h"

std::string log_regex(std::string_view level, std::string_view msg) {
  std::string regex = "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3} \\|.*\\[";
  regex += std::string(8 - level.length(), ' ');
  regex += level;
  regex += "\\].*";
  regex += msg;
  regex += ".*\n";
  return regex;
}

TEST(LoggerTest, TestBasic) {
  testing::internal::CaptureStdout();
  Logger::addConsole();
  testing::internal::GetCapturedStdout();

  testing::internal::CaptureStdout();
  LOG(INFO) << "Hello World!";
  EXPECT_THAT(testing::internal::GetCapturedStdout(),
              testing::MatchesRegex(log_regex("INFO", "Hello World!")));
}

TEST(LoggerTest, TestChangeLevel) {
  testing::internal::CaptureStdout();
  Logger::setConsoleLevel(DEBUG);
  EXPECT_THAT(
      testing::internal::GetCapturedStdout(),
      testing::
          MatchesRegex(
              "[0-9]{4}-[0-9]{2}-[0-9]{2} " "[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]" "{3} " "\\| " "Chang"
                                                                                             "ing " "log " "level" " from" " INFO" " " "t" "o" " " "D" "E" "B" "U" "G" "\n"));

  testing::internal::CaptureStdout();
  LOG(DEBUG) << "Testing Debug Msg";

  EXPECT_THAT(testing::internal::GetCapturedStdout(),
              testing::MatchesRegex(log_regex("DEBUG", "Testing Debug Msg")));
}

TEST(LoggerTest, TestDontShow) {
  testing::internal::CaptureStdout();
  LOG(TRACE) << "This should not show up";
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

TEST(LoggerTest, TestFileLogging) {
  std::filesystem::path tmp_dir = std::filesystem::temp_directory_path();

  std::filesystem::path log_path = tmp_dir / "test.log";

  // Remove the file if it exists
  std::filesystem::remove(log_path);

  Logger::addFile(log_path);

  EXPECT_TRUE(std::filesystem::exists(log_path));

  testing::internal::CaptureStdout();
  LOG(INFO) << "This should go to stdout and file";

  EXPECT_THAT(testing::internal::GetCapturedStdout(),
              testing::MatchesRegex(log_regex("INFO", "This should go to stdout and file")));

  std::ifstream log_file(log_path);
  ASSERT_TRUE(log_file.good());

  std::string last_line;
  std::string line;
  while (std::getline(log_file, line)) { last_line = line; }
  log_file.close();

  EXPECT_THAT(last_line + "\n",    // Add newline since std::getline removes it
              testing::MatchesRegex(log_regex("INFO", "This should go to stdout and file")));

  Logger::removeFile(log_path);

  testing::internal::CaptureStdout();
  LOG(INFO) << "This should go to just stdout";
  EXPECT_THAT(testing::internal::GetCapturedStdout(),
              testing::MatchesRegex(log_regex("INFO", "This should go to just stdout")));

  log_file.open(log_path);
  ASSERT_TRUE(log_file.good());

  last_line = "";
  while (std::getline(log_file, line)) { last_line = line; }
  log_file.close();

  EXPECT_THAT(last_line + "\n",    // Add newline since std::getline removes it
              testing::MatchesRegex(log_regex("INFO", "This should go to stdout and file")));
}

TEST(LoggerTest, TestCloseConsole) {
  Logger::removeConsole();

  testing::internal::CaptureStdout();
  LOG(CRITICAL) << "WE HAVE A HUGE HUGE PROBLEM AHHH";
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

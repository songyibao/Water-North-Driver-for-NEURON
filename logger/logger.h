//
// Created by root on 3/12/25.
//

#ifndef LOGGER_H
#define LOGGER_H
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iomanip>  // 支持 std::put_time 和格式控制
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>  // 支持 std::ostringstream
#include <string>
#include <thread>
class AsyncLogger {
 public:
  enum class Level { DEBUG, INFO, WARNING, ERROR };
  static AsyncLogger& GetInstance();
  void SetLevel(Level level);
  bool SetOutputFile(const std::string& filename);
  void Shutdown();
  std::string LevelToString(Level level);
  void FormatAndWrite(const std::tuple<std::chrono::system_clock::time_point,
                                       Level, std::string>& msg);
  // 日志输出函数
  void Debug(const char* format, ...);
  void Info(const char* format, ...);
  void Warning(const char* format, ...);
  void Error(const char* format, ...);
  AsyncLogger(const AsyncLogger&) = delete;
  AsyncLogger& operator=(const AsyncLogger&) = delete;

 private:
  AsyncLogger();
  ~AsyncLogger();
  void EnqueueMessage(Level level, const char* format, va_list args);
  void ProcessQueue();
  // 配置相关
  std::mutex config_mutex_;
  std::atomic<Level> level_;
  std::ofstream output_file_;
  bool output_to_file_;

  // 队列相关
  std::queue<
      std::tuple<std::chrono::system_clock::time_point, Level, std::string>>
      message_queue_;
  std::mutex queue_mutex_;
  std::atomic<bool> running_;
  std::condition_variable queue_cv_;
  std::thread worker_thread_;
};
// 简化调用的宏
#define LOG_DEBUG(...) AsyncLogger::GetInstance().Debug(__VA_ARGS__)
#define LOG_INFO(...) AsyncLogger::GetInstance().Info(__VA_ARGS__)
#define LOG_WARNING(...) AsyncLogger::GetInstance().Warning(__VA_ARGS__)
#define LOG_ERROR(...) AsyncLogger::GetInstance().Error(__VA_ARGS__)
#endif  // LOGGER_H

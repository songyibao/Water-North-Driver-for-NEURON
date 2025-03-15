//
// Created by root on 3/12/25.
//

#include "logger.h"
AsyncLogger& AsyncLogger::GetInstance()  {
  static AsyncLogger instance;
  return instance;
}
void AsyncLogger::SetLevel(Level level) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  level_ = level;
}
bool AsyncLogger::SetOutputFile(const std::string& filename) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  try {
    output_file_.open(filename, std::ios::app);
    output_to_file_ = output_file_.is_open();
    return output_to_file_;
  } catch (...) {
    output_to_file_ = false;
    return false;
  }
}
void AsyncLogger::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    running_ = false;
  }
  queue_cv_.notify_one();
  if (worker_thread_.joinable()) {
    worker_thread_.join();
  }
}
AsyncLogger::~AsyncLogger() {
  LOG_INFO("日志类析构函数被调用，正在关闭日志系统...");
  Shutdown();
  if (output_file_.is_open()) {
    output_file_.close();
  }
}
void AsyncLogger::EnqueueMessage(Level level, const char* format,
                                 va_list args) {
  if (level < level_.load(std::memory_order_relaxed)) {
    return;
  }
  char message[1024];
  vsnprintf(message, sizeof(message), format, args);
  auto now = std::chrono::system_clock::now();
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.emplace(now, level, message);
  }
  queue_cv_.notify_one();
}
void AsyncLogger::ProcessQueue() {
  while (running_ || !message_queue_.empty()) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    // 等待条件变量的通知
    queue_cv_.wait(lock, [&] { return !running_ || !message_queue_.empty(); });
    while (!message_queue_.empty()) {
      auto msg = message_queue_.front();
      message_queue_.pop();
      lock.unlock();
      FormatAndWrite(msg);
      lock.lock();
    }
  }
}
std::string  AsyncLogger::LevelToString(Level level) {
  switch (level) {
    case Level::DEBUG:
      return "DEBUG";
    case Level::INFO:
      return "INFO";
    case Level::WARNING:
      return "WARNING";
    case Level::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}
void AsyncLogger::FormatAndWrite(
    const std::tuple<std::chrono::system_clock::time_point, Level, std::string>&
        msg) {
  auto& [timestamp,level,content] = msg;
  // 转换时间戳
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch())%1000;
  std::time_t t = std::chrono::system_clock::to_time_t(timestamp);
  std::tm tm = *std::localtime(&t);

  // 格式化输出
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
      << '.' << std::setfill('0') << std::setw(3) << ms.count()
      << " [" << LevelToString(level) << "] "
      << content << "\n";

  std::string formatted = oss.str();

  // 写入输出
  {
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (output_to_file_ && output_file_.is_open()) {
      output_file_ << formatted<<std::flush;
    }else {
      std::cout<<formatted<<std::flush;
    }
  }
}

void AsyncLogger::Debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  EnqueueMessage(Level::DEBUG, format, args);
  va_end(args);
}

void AsyncLogger::Info(const char* format, ...) {
  va_list args;
  va_start(args, format);
  EnqueueMessage(Level::INFO, format, args);
  va_end(args);
}

void AsyncLogger::Warning(const char* format, ...) {
  va_list args;
  va_start(args, format);
  EnqueueMessage(Level::WARNING, format, args);
  va_end(args);
}

void AsyncLogger::Error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  EnqueueMessage(Level::ERROR, format, args);
  va_end(args);
}
AsyncLogger::AsyncLogger():running_(true),level_(Level::INFO),output_to_file_(false) {
  worker_thread_ = std::thread(&AsyncLogger::ProcessQueue, this);
}
// 使用示例
// int main() {
//   AsyncLogger::GetInstance().SetLevel(AsyncLogger::Level::DEBUG);
//   // AsyncLogger::GetInstance().SetOutputFile("app.log");
//
//   // 多线程测试
//   constexpr int NUM_THREADS = 8;
//   constexpr int LOGS_PER_THREAD = 1000;
//
//   auto worker = [] {
//     for (int i = 0; i < LOGS_PER_THREAD; ++i) {
//       LOG_INFO("Thread %d: log %d", std::this_thread::get_id(), i);
//     }
//   };
//
//   std::vector<std::thread> threads;
//   for (int i = 0; i < NUM_THREADS; ++i) {
//     threads.emplace_back(worker);
//   }
//
//   for (auto& t : threads) {
//     t.join();
//   }
//
//   AsyncLogger::GetInstance().Shutdown();
//   return 0;
// }

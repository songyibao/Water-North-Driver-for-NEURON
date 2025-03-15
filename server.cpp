//
// Created by root on 3/14/25.
//
#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include "mqtt_handle.h"
#include "mqtt_plugin.h"
#include "logger.h"
#include "internal_api/update_interval.h"
Server *Server::instance = nullptr;
Server::Server() {
  // neu_plugin_t *plugin_{};
  // // 全局消息队列
  // std::map<std::string,std::queue<std::string>> client_msg_queues_;
  // // 队列同步信号量
  // std::mutex queue_mutex_;
  //
  // static Server *instance;
  // std::atomic<bool> running{false};
  // std::thread upload_thread_;
  // std::string topic_;
  interval_ = 1000;
  south_node_num_ = 0;
  plugin_ = nullptr;
  this->running.store(false);
  topic_.clear();
}
Server::~Server() { this->Uninit(); }
// 手动提取 meter 对象（简单解析）
std::string Server::extract_meter(const std::string &json) {
  size_t meter_start = json.find("\"meter\":[") + 9;
  size_t meter_end = json.find(']', meter_start);
  plog_debug(plugin_,"提取meter成功");
  return json.substr(meter_start, meter_end - meter_start);
}

// std::queue<std::string>* Server::GetQueue(const std::string& node, const std::string& group)
int Server::HandlePushMessage(const std::string& driver_name,const std::string& group_name,const std::string& message)
{
  if (running.load()==false)
  {
    LOG_ERROR("上报进程未启动");
    return -1;
  }
  if (driver_name.empty() || group_name.empty() || message.empty())
  {
    LOG_ERROR("参数为空");
  }
  std::queue<std::string>* queue = GetQueue(driver_name,group_name);
  if (queue == nullptr)
  {
    AddNodeGroup(driver_name,group_name);

    return -1;
  }
  queue->push(message);
  return 0;
}

void Server::upload_function() {

  while (running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    size_t node_group_num = GetTotalNodeGroupPairs();
    if (node_group_msg_queue_map_.size()<node_group_num) {
      LOG_WARNING("消息队列未就绪，当前队列大小为%lu，总节点数为%lu",node_group_msg_queue_map_.size(),node_group_num);
      continue;
    }
    if (node_group_msg_queue_map_.size()>node_group_num)
    {
      LOG_ERROR("异常:消息队列大小大于总节点数，当前队列大小为%lu，总节点数为%lu",node_group_msg_queue_map_.size(),node_group_num);
      continue;
    }
    std::vector<std::string> messages;
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      for (auto &it : node_group_msg_queue_map_) {
        if (it.second.empty()) {
          // 只要有一个队列为空，就不上报数据
          messages.clear();
          break;
        }
        messages.push_back(it.second.front());
        it.second.pop();
      }
    }
    // 这里为空包含两种情况，一种是所有队列都为空，一种是有一个队列为空导致messages被清空，这两种情况都不需要上报数据
    // 即必须所有队列都不为空才上报数据
    if (messages.empty()) {
      continue;
    }
    LOG_DEBUG("开始构造上报数据");
    // 构造上报数据
    std::string upload_data;
    std::string aggregated = R"({"gateid":")" + std::string(plugin_->common.name) + R"(","time":")" +
                             GetCurrentTime() +
                             R"(","source":"da/db","meter":[)";
    for (const std::string &message : messages) {
      aggregated += extract_meter(message);
      aggregated += ',';
    }
    // 删除最后一个逗号
    aggregated.pop_back();
    aggregated += "]}";
    LOG_DEBUG("上报数据meter数组长度为%lu",messages.size());
    neu_mqtt_qos_e qos   = plugin_->config.qos;
    int rv = publish(plugin_, qos, const_cast<char *>(topic_.c_str()), aggregated.c_str(), aggregated.length());
    if (rv <0)
    {
      plog_error(plugin_,"MQTT数据发布失败");
    }
    LOG_DEBUG("上报数据到Topic:%s, 成功",topic_.c_str());
  }
  LOG_DEBUG("上报线程退出");
}

std::string Server::GetCurrentTime()
{
  // 获取当前时间戳
  std::time_t now = std::time(nullptr);

  // 线程安全的时间结构体
  std::tm tm_struct{};

  localtime_r(&now, &tm_struct); // POSIX版本（Linux/macOS）

  // 格式化成字符串
  char buffer[20];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_struct);

  return buffer;
}

Server *Server::get_instance() {
  static Server server_instance;
  return &server_instance;
}

int Server::Init() {
  return 0;
}

int Server::Uninit() {
  this->Stop();
  return 0;
}

int Server::Start()
{
  if (topic_.empty())
  {
    plog_error(plugin_,"未设置mqtt上传主题");
    return -1;
  }
  if (plugin_ == nullptr)
  {
    plog_error(plugin_,"未设置plugin指针");
    return -1;
  }
  // 启动标志
  running.store(true);
  // 日志类初始化
  AsyncLogger::GetInstance().SetLevel(AsyncLogger::Level::DEBUG);
  UpdateAllNodeGroupInterval();

  upload_thread_ = std::thread([this]
  {
    upload_function();
  });
  // interval_monitor_thread_ = std::thread([this]
  // {
  //   while (running.load()) {
  //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
  //     UpdateAllNodeGroupInterval();
  //   }
  // });

  return 0;
}

int Server::Stop() {
  running.store(false);
  if (upload_thread_.joinable()) {
    upload_thread_.join();
  }
  if (interval_monitor_thread_.joinable()) {
    interval_monitor_thread_.join();
  }
  // for (auto &it : threads) {
  //   if (it.joinable()) {
  //     it.join();
  //   }
  // }
  // this->threads.clear();
  // AsyncLogger::GetInstance().Shutdown();
  return 0;
}

std::string& Server::GetTopic()
{
  return topic_;
}

uint32_t Server::GetInterval()
{
  return interval_;
}

void Server::SetPlugin(neu_plugin_t* plugin)
{
  plugin_ = plugin;
}

void Server::SetTopic(const std::string& topic)
{
  topic_ = topic;
}

void Server::SetInterval(uint32_t interval)
{
  if (interval<200)
  {
    interval = 200;
  }
  if (interval>10000)
  {
    interval = 10000;
  }
  interval_ = interval;
  UpdateAllNodeGroupInterval();
}

void Server::UpdateAllNodeGroupInterval(bool use_thread)
{
  for (auto &it : node_group_data_) {
    for (const auto &group : it.second) {
      if (use_thread)
      {
        std::thread tmp(update_interval,std::string(it.first),std::string(group),(int)interval_,plugin_);
        tmp.detach();
      }else
      {
        update_interval(std::string(it.first),std::string(group),(int)interval_,plugin_);
      }
    }
  }
}

void Server::UpdateNodeGroupInterval(const std::string& node, const std::string& group)
{
  if (node.empty() || group.empty())
  {
    LOG_ERROR("节点名或组名不能为空");
    return;
  }
  std::thread tmp(update_interval,std::string(node),std::string(group),(int)interval_,plugin_);
  tmp.detach();
}

void Server::UpdateGroup(const std::string& node, const std::string& group, const std::string& new_group,uint32_t interval)
{
  LOG_DEBUG("更新组名，node:%s,group:%s,new_group:%s,interval:%lu",node.c_str(),group.c_str(),new_group.c_str(),interval);
  if (group.empty() || new_group.empty())
  {
    LOG_ERROR("组名不能为空");
    return;
  }
  if (group==new_group)
  {
    LOG_ERROR("更新前后组名不能相同");
    return;
  }
  size_t old_size = GetTotalNodeGroupPairs();
  auto it = node_group_data_.find(node);
  if (it != node_group_data_.end()) { // 确保 node 存在
    if (it->second.erase(group)) { // 如果 group 存在，删除它
      auto old_key = std::make_pair(node, group);
      auto new_key = std::make_pair(node, new_group);
      if (node_group_msg_queue_map_.find(old_key)!=node_group_msg_queue_map_.end())
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        node_group_msg_queue_map_[new_key] = std::move(node_group_msg_queue_map_[old_key]); // 移动消息队列
        node_group_msg_queue_map_.erase(old_key); // 删除旧的消息队列
      }
      it->second.insert(new_group); // 插入新 group
    }
  }
  if (old_size!=GetTotalNodeGroupPairs())
  {
    LOG_ERROR("异常：更新组名前后节点组对数量不一致");
  }
}

void Server::UpdateNode(const std::string& node, const std::string& new_node)
{
  LOG_DEBUG("更新节点名，node:%s,new_node:%s",node.c_str(),new_node.c_str());
  if (node.empty() || new_node.empty())
  {
    LOG_ERROR("节点名不能为空");
    return;
  }
  if (node==new_node)
  {
    LOG_ERROR("更新前后节点名不能相同");
    return;
  }
  auto it = node_group_data_.find(node);
  if (it != node_group_data_.end()) { // 确保 node 存在
    GroupSet groups = std::move(it->second); // 移动 groups 到临时变量
    node_group_data_.erase(it); // 删除原 node
    node_group_data_[new_node] = std::move(groups); // 将 groups 赋值给 new_node
    // 移动消息队列
    for (const auto&group:groups)
    {
      auto old_key = std::make_pair(node, group);
      auto new_key = std::make_pair(new_node, group);
      if (node_group_msg_queue_map_.find(old_key)!=node_group_msg_queue_map_.end())
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        node_group_msg_queue_map_[new_key] = std::move(node_group_msg_queue_map_[old_key]); // 移动消息队列
        node_group_msg_queue_map_.erase(old_key); // 删除旧的消息队列
      }
    }
  }

}

void Server::AddNodeGroup(const std::string& node, const std::string& group)
{
  if (node.empty() || group.empty())
  {
    LOG_ERROR("节点名或组名不能为空");
    return;
  }
  auto [it, inserted] = node_group_data_[node].insert(group); // 如果 node 不存在，会自动创建
  if (inserted) {
    // 原来该组不存在，且插入成功，创建新的消息队列
    std::lock_guard<std::mutex> lock(queue_mutex_);
    node_group_msg_queue_map_[{node, group}] = std::queue<std::string>(); // 创建新的消息队列
  }
  UpdateNodeGroupInterval(node,group);
}

void Server::RemoveNodeGroup(const std::string& node, const std::string& group)
{
  if (node.empty() || group.empty())
  {
    LOG_ERROR("节点名或组名不能为空");
    return;
  }
  auto it = node_group_data_.find(node);
  if (it != node_group_data_.end()) { // 确保 node 存在
    if (it->second.erase(group)) {
      {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        node_group_msg_queue_map_.erase({node, group}); // 删除消息队列
      }
      if (it->second.empty()) {
        node_group_data_.erase(it); // 如果 group 集合为空，删除 node
      }
    }
  }
}

void Server::RemoveNode(const std::string& node)
{
  if (node.empty())
  {
    LOG_ERROR("节点名不能为空");
    return;
  }
  auto it = node_group_data_.find(node);
  if (it != node_group_data_.end()) {
    // 获取该 node 下的所有 group
    const auto& groups = it->second;
    for (const auto& group : groups) {
      auto key = std::make_pair(node, group);
      std::lock_guard<std::mutex> lock(queue_mutex_);
      node_group_msg_queue_map_.erase(key); // 删除对应的消息队列
    }
    // 删除 data 中的 node
    node_group_data_.erase(it);
  }
}

std::queue<std::string>* Server::GetQueue(const std::string& node, const std::string& group)
{
  if (node.empty() || group.empty())
  {
    LOG_ERROR("节点名或组名不能为空");
    return nullptr;
  }
  auto key = std::make_pair(node, group);
  auto it = node_group_msg_queue_map_.find(key);
  if (it != node_group_msg_queue_map_.end()) {
    return &it->second; // 返回消息队列的指针
  }
  return nullptr; // 如果不存在，返回 nullptr
}

size_t Server::GetTotalNodeGroupPairs() const
{
  size_t total = 0;
  for (const auto& pair : node_group_data_) {
    total += pair.second.size(); // pair.second 是 group 集合
  }
  return total;
}

void Server::AddOneSouthNode()
{
  south_node_num_++;
}

void Server::DeleteOneSouthNode()
{
  south_node_num_--;
}

bool Server::GetRunningStatus()
{
  return running.load();
}

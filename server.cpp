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

#include "internal_api/update_interval.h"
#include "mqtt_handle.h"
#include "mqtt_plugin.h"
Server* Server::instance = nullptr;
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
    regions_count = -1;
    interval_ = 1000;
    plugin_ = nullptr;
    this->running.store(false);
    pub_topic_.clear();
}
Server::~Server() { this->Uninit(); }
// 手动提取 meter 对象（简单解析）
std::string Server::extract_meter(const std::string& json) {
    size_t meter_start = json.find("\"meter\":[") + 9;
    size_t meter_end = json.find(']', meter_start);
    plog_debug(plugin_, "提取meter成功");
    return json.substr(meter_start, meter_end - meter_start);
}

void Server::upload_thread_function() {
    plog_debug(plugin_, "新上报线程开始");

    while (running.load()) {
        if (regions_count == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        msg_map_buffer_.clear();

        // 条件等待数据准备完成
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        buffer_cv_.wait(lock, [this] {
            plog_debug(plugin_, "条件变量等待中");
            return msg_map_buffer_.size() >= regions_count || !running.load();
        });
        if (!running.load()) {
            plog_debug(plugin_, "启动位置为false，退出上报线程");
            break;
        }
        if (msg_map_buffer_.size() > regions_count) {
            plog_error(plugin_,"异常:消息缓冲区大小大于最大值，当前大小为%lu", msg_map_buffer_.size());
            // 清空缓冲区
            msg_map_buffer_.clear();
            continue;
        }
        plog_notice(plugin_, "开始构造上报数据");
        plog_notice(plugin_, "上报数据meter数组长度为%lu", msg_map_buffer_.size());

        // 构造上报数据 - 不需要再加锁，因为已经持有锁
        std::string upload_data;
        std::string aggregated = R"({"gateid":")" + std::string(plugin_->common.name) + R"(","time":")" + GetCurrentTime() + R"(","source":"da/db","meter":[)";
        for (auto& item : msg_map_buffer_) {
            aggregated += extract_meter(item.second);
            aggregated += ',';
        }
        // 删除最后一个逗号
        if (!msg_map_buffer_.empty()) {
            // 添加检查防止空缓冲区,非空时才会有最后一个逗号
            aggregated.pop_back();
        }
        aggregated += "]}";

        // lock.unlock();  // 不需要手动解锁，unique_lock会在作用域结束时自动解锁

        neu_mqtt_qos_e qos = plugin_->config.qos;
        int rv = publish(plugin_, qos, const_cast<char*>(pub_topic_.c_str()), aggregated.c_str(), aggregated.length());
        if (rv < 0) {
            plog_error(plugin_, "MQTT数据发布失败");
        }
        plog_notice(plugin_, "上报数据到Topic:%s, success", pub_topic_.c_str());
    }
    plog_notice(plugin_, "上报线程退出");
}
std::string Server::GetCurrentTime() {
    // 获取当前时间戳
    std::time_t now = std::time(nullptr);

    // 线程安全的时间结构体
    std::tm tm_struct{};

    localtime_r(&now, &tm_struct);  // POSIX版本（Linux/macOS）

    // 格式化成字符串
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_struct);

    return buffer;
}
void Server::AddToBuffer(char* key, char* msg) {
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (msg_map_buffer_.size() >= regions_count) {
            plog_error(plugin_, "错误,消息缓冲区已满");
            msg_map_buffer_.clear();
            return;
        }
        // msg_buffer_.emplace_back(msg);
        // 允许覆盖
        msg_map_buffer_[key] = msg;
    }
    // 通知条件变量
    buffer_cv_.notify_one();
}

Server* Server::get_instance() {
    static Server server_instance;
    return &server_instance;
}

int Server::Init() { return 0; }

int Server::Uninit() {
    this->Stop();
    return 0;
}

int Server::Start() {
    plog_debug(plugin_, "启动上报服务");
    if (pub_topic_.empty()) {
        plog_error(plugin_, "未设置mqtt上传主题");
        return -1;
    }
    if (plugin_ == nullptr) {
        plog_error(plugin_, "未设置plugin指针");
        return -1;
    }
    plog_info(plugin_, "设置启动标志");
    // 启动标志
    running.store(true);

    plog_info(plugin_, "更新所有节点间隔");
    UpdateAllNodeGroupInterval();

    plog_info(plugin_, "节点数%lu", GetTotalNodeGroupPairs());

    plog_notice(plugin_, "启动上报线程");
    upload_thread_ = std::thread([this] { upload_thread_function(); });

    // plog_notice(plugin_, "启动采集间隔监测线程");
    // interval_monitor_thread_ = std::thread([this]
    // {
    //   while (running.load()) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //     UpdateAllNodeGroupInterval();
    //   }
    // });

    return 0;
}

int Server::Stop() {
    running.store(false);
    buffer_cv_.notify_all();

    // 放在最后部分
    if (upload_thread_.joinable()) {
        upload_thread_.join();
    }
    if (interval_monitor_thread_.joinable()) {
        interval_monitor_thread_.join();
    }
    return 0;
}

std::string& Server::getPubTopic() { return pub_topic_; }
std::string& Server::getSubTopic() { return sub_topic_; }

uint32_t Server::getInterval() { return interval_; }

void Server::setPlugin(neu_plugin_t* plugin) { plugin_ = plugin; }

void Server::setPubTopic(const std::string& topic) { pub_topic_ = topic; }

void Server::setSubTopic(const std::string& topic) { sub_topic_ = topic; }
void Server::setInterval(uint32_t interval) {
    if (interval < 500) {
        interval = 500;
    }
    if (interval > 10000) {
        interval = 10000;
    }
    interval_ = interval;
    UpdateAllNodeGroupInterval();
}

void Server::UpdateAllNodeGroupInterval() {
    for (auto& it : node_group_data_) {
        for (const auto& group : it.second) {
            send_update_group_interval_req(std::string(it.first), std::string(group), (int)interval_, plugin_);
        }
    }
}

void Server::UpdateNodeGroupInterval(const std::string& node, const std::string& group) {
    if (node.empty() || group.empty()) {
        plog_error(plugin_,"节点名或组名不能为空");
        return;
    }
    std::thread tmp(send_update_group_interval_req, std::string(node), std::string(group), (int)interval_, plugin_);
    tmp.detach();
}

void Server::UpdateGroupName(const std::string& node, const std::string& group, const std::string& new_group) {
    plog_notice(plugin_,"更新组名，node:%s,group:%s,new_group:%s", node.c_str(), group.c_str(), new_group.c_str());
    if (group.empty() || new_group.empty()) {
        plog_error(plugin_, "组名不能为空");
        return;
    }
    if (group == new_group) {
        plog_warn(plugin_, "新旧组名相同,无需更新");
        return;
    }
    size_t old_size = GetTotalNodeGroupPairs();
    auto it = node_group_data_.find(node);
    if (it != node_group_data_.end()) {    // 确保 node 存在
        if (it->second.erase(group)) {     // 如果 group 存在，删除它
            it->second.insert(new_group);  // 插入新 group
        }
    }
    if (old_size != GetTotalNodeGroupPairs()) {
        plog_error(plugin_,"异常：更新组名前后节点组对数量不一致");
    }
}

void Server::UpdateNodeName(const std::string& node, const std::string& new_node) {
    plog_error(plugin_,"更新节点名，node:%s,new_node:%s", node.c_str(), new_node.c_str());
    if (node.empty() || new_node.empty()) {
        plog_error(plugin_,"节点名不能为空");
        return;
    }
    if (node == new_node) {
        plog_error(plugin_,"更新前后节点名不能相同");
        return;
    }
    auto it = node_group_data_.find(node);
    if (it != node_group_data_.end()) {                  // 确保 node 存在
        GroupSet groups = std::move(it->second);         // 移动 groups 到临时变量
        node_group_data_.erase(it);                      // 删除原 node
        node_group_data_[new_node] = std::move(groups);  // 将 groups 赋值给 new_node
    }
}

int Server::AddNodeGroup(const std::string& node, const std::string& group) {
    if (node.empty() || group.empty()) {
        plog_error(plugin_,"节点名或组名不能为空");
        return -1;
    }
    auto [it, inserted] = node_group_data_[node].insert(group);  // 如果 node 不存在，会自动创建
    UpdateNodeGroupInterval(node, group);
    return 0;  // 返回0表示成功
}

void Server::RemoveNodeGroup(const std::string& node, const std::string& group) {
    if (node.empty() || group.empty()) {
        plog_error(plugin_,"节点名或组名不能为空");
        return;
    }
    auto it = node_group_data_.find(node);
    if (it != node_group_data_.end()) {  // 确保 node 存在
        if (it->second.erase(group)) {
            if (it->second.empty()) {
                node_group_data_.erase(it);  // 如果 group 集合为空，删除 node
            }
        }
    }
}

void Server::RemoveNode(const std::string& node) {
    if (node.empty()) {
        plog_error(plugin_,"节点名不能为空");
        return;
    }
    auto it = node_group_data_.find(node);
    if (it != node_group_data_.end()) {
        // 删除 data 中的 node
        node_group_data_.erase(it);
    }
}

size_t Server::GetTotalNodeGroupPairs() const {
    size_t total = 0;
    for (const auto& pair : node_group_data_) {
        total += pair.second.size();  // pair.second 是 group 集合
    }
    return total;
}

bool Server::GetRunningStatus() { return running.load(); }

//
// Created by root on 3/14/25.
//

#ifndef CPP_DATA_H
#define CPP_DATA_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mqtt_plugin.h"
class Server {
   public:
    int regions_count = -1;
    // 消息缓冲区新版本,使用map,每个节点有自己的固定位置缓冲区
    std::unordered_map<std::string, std::string> msg_map_buffer_;
    // 消息缓冲区同步信号量
    std::mutex buffer_mutex_;
    // 缓冲区条件变量
    std::condition_variable buffer_cv_;

   private:
    neu_plugin_t* plugin_;
    using GroupSet = std::unordered_set<std::string>;
    using NodeMap = std::unordered_map<std::string, GroupSet>;
    static Server* instance;

    NodeMap node_group_data_;

    std::atomic<bool> running{false};
    std::thread upload_thread_;
    std::thread interval_monitor_thread_;
    std::string pub_topic_;
    std::string sub_topic_;

    uint32_t interval_;
    Server();

   public:
    void operator=(const Server&) = delete;
    Server(const Server&) = delete;
    Server(Server&& other_server) = delete;
    ~Server();
    static Server* get_instance();

    int Init();
    int Uninit();
    int Start();
    int Stop();

    // getters
    std::string& getPubTopic();
    std::string& getSubTopic();
    uint32_t getInterval();

    // setters
    void setPlugin(neu_plugin_t*);
    void setPubTopic(const std::string& topic);
    void setSubTopic(const std::string& topic);
    void setInterval(uint32_t interval);

    // services
    void UpdateAllNodeGroupInterval();
    void UpdateNodeGroupInterval(const std::string& node, const std::string& group);
    void UpdateGroupName(const std::string& node, const std::string& group, const std::string& new_group);
    void UpdateNodeName(const std::string& node, const std::string& new_node);
    int AddNodeGroup(const std::string& node, const std::string& group);
    void RemoveNodeGroup(const std::string& node, const std::string& group);
    void RemoveNode(const std::string& node);

    size_t GetTotalNodeGroupPairs() const;
    bool GetRunningStatus();
    std::string extract_meter(const std::string& json);
    void upload_thread_function();
    std::string GetCurrentTime();

    // msg_buffer_
    void AddToBuffer(char* key, char* msg);
};

#endif  // CPP_DATA_H

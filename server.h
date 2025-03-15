//
// Created by root on 3/14/25.
//

#ifndef CPP_DATA_H
#define CPP_DATA_H
#include <string>
#include <thread>
#include <atomic>
#include<vector>
#include<queue>
#include<mutex>
#include<map>
#include<unordered_set>
#include<unordered_map>
#include"mqtt_plugin.h"
#include <set>
#define MAX_CLIENTS 5;
class Server {
private:
    neu_plugin_t *plugin_;
    using GroupSet = std::unordered_set<std::string>;
    using NodeMap = std::unordered_map<std::string, GroupSet>;
    // 自定义哈希函数
    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };
    using QueueMap = std::unordered_map<std::pair<std::string, std::string>, std::queue<std::string>,PairHash>;


    // 全局消息队列
    QueueMap node_group_msg_queue_map_;
    // 队列同步信号量
    std::mutex queue_mutex_;

    static Server *instance;



    NodeMap node_group_data_;

    std::atomic<bool> running{false};
    std::thread upload_thread_;
    std::thread interval_monitor_thread_;
    std::string topic_;
    int south_node_num_;
    uint32_t interval_;
    Server();
public:
    void operator=(const Server &) = delete;
    Server(const Server &) = delete;
    Server(Server &&other_server)  = delete;
    ~Server();
    static Server* get_instance();


    int Init();
    int Uninit();
    int Start();
    int Stop();

    //getters
    std::string& GetTopic();
    uint32_t GetInterval();

    // setters
    void SetPlugin(neu_plugin_t *);
    void SetTopic(const std::string &topic);
    void SetInterval(uint32_t interval);

    // services
    void UpdateAllNodeGroupInterval(bool use_thread=true);
    void UpdateNodeGroupInterval(const std::string& node, const std::string& group);
    void UpdateGroup(const std::string& node, const std::string& group,const std::string& new_group,uint32_t interval);
    void UpdateNode(const std::string& node,const std::string& new_node);
    void AddNodeGroup(const std::string& node, const std::string& group);
    void RemoveNodeGroup(const std::string& node, const std::string& group);
    void RemoveNode(const std::string& node);

    // 获取 {node, group} 的消息队列指针
    std::queue<std::string>* GetQueue(const std::string& node, const std::string& group);
    size_t GetTotalNodeGroupPairs() const;
    void AddOneSouthNode();
    void DeleteOneSouthNode();
    bool GetRunningStatus();
    std::string extract_meter(const std::string& json);
    int HandlePushMessage(const std::string& driver_name,const std::string& group_name,const std::string&);
    void upload_function();
    std::string GetCurrentTime();
};


#endif //CPP_DATA_H

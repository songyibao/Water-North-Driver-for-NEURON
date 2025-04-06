#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <cstring>  // for memset
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "../mqtt_plugin.h"

int send_read_group_req(neu_plugin_t* plugin, const std::string driver, const std::string& group) {
    neu_reqresp_head_t header = {.type = NEU_REQ_READ_GROUP, .ctx = NULL};

    neu_req_read_group_t read_cmd = {.sync = true};

    // 使用 strdup 简化字符串复制（自动计算长度 + malloc）
    read_cmd.driver = strdup(driver.c_str());
    read_cmd.group = strdup(group.c_str());

    if (0 != neu_plugin_op(plugin, header, &read_cmd)) {
        plog_error(plugin, "neu_plugin_op(NEU_REQ_READ_GROUP) fail");
        // 假设 neu_plugin_op 内部会释放内存，否则需要手动 free
        return -1;
    }

    plog_notice(plugin, "send read group request success");
    return 0;
}

int send_update_group_interval_req(const std::string& node, const std::string& group, int interval, neu_plugin_t* plugin) {
    plog_notice(plugin, "update group interval, group:%s, node:%s", group.c_str(), node.c_str());

    neu_reqresp_head_t header = {};
    neu_req_update_group_t cmd = {};

    header.ctx = NULL;
    header.type = NEU_REQ_UPDATE_GROUP;
    header.otel_trace_type = NEU_OTEL_TRACE_TYPE_REST_COMM;

    // 根据上面的定义构造请求体
    strncpy(cmd.driver, node.c_str(), sizeof(cmd.driver) - 1);
    strncpy(cmd.group, group.c_str(), sizeof(cmd.group) - 1);
    strncpy(cmd.new_name, group.c_str(), sizeof(cmd.new_name) - 1);
    cmd.interval = interval;

    plog_debug(plugin, "sending update group interval request, driver:%s, group:%s, interval:%d", cmd.driver, cmd.group, cmd.interval);

    // 发起请求
    if (0 != neu_plugin_op(plugin, header, &cmd)) {
        plog_error(plugin, "neu_plugin_op(NEU_REQ_UPDATE_DRIVER_GROUP) fail,driver:%s, group:%s", node.c_str(), group.c_str());
        return -1;
    }
    plog_notice(plugin, "send update group interval request success, driver:%s, group:%s", node.c_str(), group.c_str());
    return 0;
}
std::string http_get_interval(const std::string& node,const std::string &host,uint16_t port) {

    // 构造 HTTP 请求
    std::ostringstream request_stream;
    request_stream << "GET /api/v2/group?node="+node+" HTTP/1.1\r\n";
    request_stream << "Host: "+host+":"+std::to_string(port)+"\r\n";
    // request_stream << "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw\r\n";
    request_stream << "Connection: close\r\n\r\n";
    // request_stream << "Content-Type: application/json\r\n";
    // request_stream << "Content-Length: " << json_body.length() << "\r\n";
    // request_stream << "\r\n";
    // request_stream << json_body;

    std::string request = request_stream.str();

    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "无法创建套接字" << std::endl;
        return "";
    }

    // 连接到服务器
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(host.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "连接失败" << std::endl;
        close(sock);
        return "";
    }
    // std::cout<<"连接成功"<<std::endl;
    // 发送请求
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "发送失败" << std::endl;
        close(sock);
        return "";
    }
    // std::cout<<"发送成功"<<std::endl;
    // 可选：读取响应并检查状态码
    char buffer[1024];
    std::string response;
    int bytes_received;
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        response += buffer;
    }

    if (bytes_received < 0) {
        std::cerr << "接收失败" << std::endl;
        close(sock);
        return "";
    }
    // std::cout<<"接收成功"<<std::endl;
    close(sock);
    return response;
}
// 获取指定 group 的 interval
int GetGroupInterval(const std::string& node, const std::string& group) {
    std::string response = http_get_interval(node,"127.0.0.1",7000);
    // std::cout<<response<<std::endl;
    if (response.find("Error:") != std::string::npos) {
        return -1; // 请求失败
    }

    // 解析 JSON 响应（简化版，假设格式固定）
    size_t group_pos = response.find("\"name\": \"" + group + "\"");
    if (group_pos == std::string::npos) {
        std::cout<<"未找到group"<<std::endl;
        return -1; // Group 不存在
    }

    size_t interval_pos = response.find("\"interval\":", group_pos);
    if (interval_pos == std::string::npos) {
        std::cout<<"未找到interval"<<std::endl;
        return -1; // Interval 字段不存在
    }

    // 提取纯数字部分
    size_t start = response.find_first_of("0123456789", interval_pos);
    if (start == std::string::npos) {
        return -1;
    }
    size_t end = response.find_first_not_of("0123456789", start);
    return std::stoi(response.substr(start, end - start));
}

// int main() {
//     int res = GetGroupInterval("LowPressurization","1");
//     // std::cout<<res<<std::endl;
//     return 0;
// }
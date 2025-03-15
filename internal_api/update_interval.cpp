#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../mqtt_plugin.h"

int update_interval(std::string&& node, std::string&& group, int interval, neu_plugin_t* plugin) {
    // 构造 JSON 数据
    std::string json_body = R"({"node":")" + node + R"(","group":")" + group + R"(","interval": )"+std::to_string(interval)+"}";

    // 构造 HTTP 请求
    std::ostringstream request_stream;
    request_stream << "PUT /api/v2/group HTTP/1.1\r\n";
    request_stream << "Host: 127.0.0.1:7000\r\n";
    // request_stream << "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw\r\n";
    request_stream << "User-Agent: Apifox/1.0.0 (https://apifox.com)\r\n";
    request_stream << "Content-Type: application/json\r\n";
    request_stream << "Content-Length: " << json_body.length() << "\r\n";
    request_stream << "\r\n";
    request_stream << json_body;

    std::string request = request_stream.str();

    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "无法创建套接字" << std::endl;
        return -1;
    }

    // 连接到服务器
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("192.168.5.27");
    server.sin_family = AF_INET;
    server.sin_port = htons(7000);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "连接失败" << std::endl;
        close(sock);
        return -1;
    }
    std::cout<<"连接成功"<<std::endl;
    // 发送请求
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "发送失败" << std::endl;
        close(sock);
        return -1;
    }
    std::cout<<"发送成功"<<std::endl;
    return 0;
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
        return -1;
    }
    std::cout<<"接收成功"<<std::endl;
    // 解析响应首行，获取状态码
    size_t end_of_first_line = response.find("\r\n");
    if (end_of_first_line != std::string::npos) {
        std::string first_line = response.substr(0, end_of_first_line);
        size_t space_pos = first_line.find(' ');
        if (space_pos != std::string::npos) {
            size_t second_space_pos = first_line.find(' ', space_pos + 1);
            if (second_space_pos != std::string::npos) {
                std::string status_code_str = first_line.substr(space_pos + 1, second_space_pos - space_pos - 1);
                int status_code = std::stoi(status_code_str);
                if (status_code >= 200 && status_code < 300) {
                    // 请求成功
                    close(sock);
                    return 0;
                } else {
                    std::cerr << "请求失败，状态码: " << status_code << std::endl;
                    close(sock);
                    return -1;
                }
            }
        }
    }

    std::cerr << "无法解析响应" << std::endl;
    close(sock);
    return -1;
}
int update_interval(std::string& node, std::string& group, int interval, neu_plugin_t* plugin) {
    // 构造 JSON 数据
    std::string json_body = R"({"node":")" + node + R"(","group":")" + group + R"(","interval": )"+std::to_string(interval)+"}";

    // 构造 HTTP 请求
    std::ostringstream request_stream;
    request_stream << "PUT /api/v2/group HTTP/1.1\r\n";
    request_stream << "Host: 127.0.0.1:7000\r\n";
    // request_stream << "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw\r\n";
    request_stream << "User-Agent: Apifox/1.0.0 (https://apifox.com)\r\n";
    request_stream << "Content-Type: application/json\r\n";
    request_stream << "Content-Length: " << json_body.length() << "\r\n";
    request_stream << "\r\n";
    request_stream << json_body;

    std::string request = request_stream.str();

    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "无法创建套接字" << std::endl;
        return -1;
    }

    // 连接到服务器
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("192.168.5.27");
    server.sin_family = AF_INET;
    server.sin_port = htons(7000);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "连接失败" << std::endl;
        close(sock);
        return -1;
    }
    std::cout<<"连接成功"<<std::endl;
    // 发送请求
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        std::cerr << "发送失败" << std::endl;
        close(sock);
        return -1;
    }
    std::cout<<"发送成功"<<std::endl;
    return 0;
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
        return -1;
    }
    std::cout<<"接收成功"<<std::endl;
    // 解析响应首行，获取状态码
    size_t end_of_first_line = response.find("\r\n");
    if (end_of_first_line != std::string::npos) {
        std::string first_line = response.substr(0, end_of_first_line);
        size_t space_pos = first_line.find(' ');
        if (space_pos != std::string::npos) {
            size_t second_space_pos = first_line.find(' ', space_pos + 1);
            if (second_space_pos != std::string::npos) {
                std::string status_code_str = first_line.substr(space_pos + 1, second_space_pos - space_pos - 1);
                int status_code = std::stoi(status_code_str);
                if (status_code >= 200 && status_code < 300) {
                    // 请求成功
                    close(sock);
                    return 0;
                } else {
                    std::cerr << "请求失败，状态码: " << status_code << std::endl;
                    close(sock);
                    return -1;
                }
            }
        }
    }

    std::cerr << "无法解析响应" << std::endl;
    close(sock);
    return -1;
}

// int main()
// {
//     update_interval("南向","group1",800,NULL);
//     return 0;
// }
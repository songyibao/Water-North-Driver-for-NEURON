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

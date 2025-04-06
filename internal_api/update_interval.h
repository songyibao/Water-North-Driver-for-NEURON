//
// Created by root on 3/14/25.
//

#ifndef WATER_PLUGIN_UPDATE_INTERVAL_H
#define WATER_PLUGIN_UPDATE_INTERVAL_H
#include "../mqtt_plugin.h"
int send_update_group_interval_req(const std::string& node,const std::string& group, int interval, neu_plugin_t* plugin);
int send_read_group_req(neu_plugin_t* plugin, const std::string driver,const std::string& group);
int GetGroupInterval(const std::string& node, const std::string& group);
#endif //WATER_PLUGIN_UPDATE_INTERVAL_H

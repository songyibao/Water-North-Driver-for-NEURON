//
// Created by root on 3/14/25.
//

#ifndef WATER_PLUGIN_UPDATE_INTERVAL_H
#define WATER_PLUGIN_UPDATE_INTERVAL_H
#include "../mqtt_plugin.h"
int update_interval(std::string&& node, std::string&& group, int interval, neu_plugin_t* plugin);
// int update_interval(std::string& node, std::string& group, int interval, neu_plugin_t* plugin);
#endif //WATER_PLUGIN_UPDATE_INTERVAL_H

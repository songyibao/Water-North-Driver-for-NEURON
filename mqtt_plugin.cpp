/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2024 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/
#include "mqtt_plugin.h"

const neu_plugin_intf_funs_t mqtt_plugin_intf_funs = {
    .open = mqtt_plugin_open,
    .close = mqtt_plugin_close,
    .init = mqtt_plugin_init,
    .uninit = mqtt_plugin_uninit,
    .start = mqtt_plugin_start,
    .stop = mqtt_plugin_stop,
    .setting = mqtt_plugin_config,
    .request = mqtt_plugin_request,
};

const neu_plugin_module_t neu_plugin_module = {
    .version = NEURON_PLUGIN_VER_1_0,
    .schema = "water",
    .module_name = "水房数据上报",
    .module_descr = DESCRIPTION,
    .module_descr_zh = DESCRIPTION_ZH,
    .intf_funs = &mqtt_plugin_intf_funs,
    .type = NEU_NA_TYPE_APP,
    .kind = NEU_PLUGIN_KIND_SYSTEM,
    .display = true,
    .single = false,
};

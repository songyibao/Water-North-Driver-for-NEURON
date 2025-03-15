//
// Created by root on 3/7/25.
//

#include "json_transform.h"
#include "yyjson.h"
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
void process_value(yyjson_mut_doc *original_mut_doc, yyjson_mut_doc *template_mut_doc, yyjson_mut_val *template_cur_root, yyjson_mut_val *cur_root_parent, size_t cur_template_root_idx, yyjson_mut_val *cur_template_root_key, bool is_array) {
    if (yyjson_mut_is_str(template_cur_root)) {
        const char *template_str = yyjson_mut_get_str(template_cur_root);
        yyjson_ptr_err err;
        yyjson_mut_val *val = yyjson_mut_doc_ptr_get(original_mut_doc, template_str);
        yyjson_mut_val *new_val = yyjson_mut_val_mut_copy(template_mut_doc, val);
        // // 处理 time 字段的时间戳转换
        // if (cur_template_root_key != NULL && strcmp(yyjson_mut_get_str(cur_template_root_key), "time") == 0) {
        //     if (yyjson_mut_is_num(new_val)) {
        //         time_t timestamp = yyjson_mut_get_int(new_val);
        //         const char *formatted_time = timestamp_to_date_string(timestamp);
        //
        //         // 创建新的字符串值并替换旧的时间戳
        //         yyjson_mut_val *formatted_val = yyjson_mut_str(template_mut_doc, formatted_time);
        //         if (is_array) {
        //             yyjson_mut_arr_replace(cur_root_parent, cur_template_root_idx, formatted_val);
        //         } else {
        //             yyjson_mut_obj_replace(cur_root_parent, cur_template_root_key, formatted_val);
        //         }
        //         return;
        //     }
        // }
        if (is_array) {
            yyjson_mut_arr_replace(cur_root_parent, cur_template_root_idx, new_val);
        } else {
            yyjson_mut_obj_replace(cur_root_parent, cur_template_root_key, new_val);
        }
    } else if (yyjson_mut_is_obj(template_cur_root)) {
        size_t idx, max;
        yyjson_mut_val *key, *val;
        yyjson_mut_obj_foreach(template_cur_root, idx, max, key, val) {
            process_value(original_mut_doc, template_mut_doc, val, template_cur_root, idx, key, false);
        }
    } else if (yyjson_mut_is_arr(template_cur_root)) {
        size_t idx, max;
        yyjson_mut_val *val;
        yyjson_mut_arr_foreach(template_cur_root, idx, max, val) {
            process_value(original_mut_doc, template_mut_doc, val, template_cur_root, idx, NULL, true);
        }
    }
}

void start_process(yyjson_mut_doc *original_mut_doc, yyjson_mut_doc *template_mut_doc) {
    yyjson_mut_val *template_mut_root = yyjson_mut_doc_get_root(template_mut_doc);

    if (yyjson_mut_is_obj(template_mut_root)) {
        size_t idx, max;
        yyjson_mut_val *key, *val;
        yyjson_mut_obj_foreach(template_mut_root, idx, max, key, val) {
            process_value(original_mut_doc, template_mut_doc, val, template_mut_root, idx, key, false);
        }
    } else if (yyjson_mut_is_arr(template_mut_root)) {
        size_t idx, max;
        yyjson_mut_val *val;
        yyjson_mut_arr_foreach(template_mut_root, idx, max, val) {
            process_value(original_mut_doc, template_mut_doc, val, template_mut_root, idx, NULL, true);
        }
    } else if (yyjson_mut_is_str(template_mut_root)) {
        const char *template_str = yyjson_mut_get_str(template_mut_root);
        yyjson_mut_val *new_val = yyjson_mut_val_mut_copy(template_mut_doc, yyjson_mut_doc_ptr_get(original_mut_doc, template_str));
        yyjson_mut_doc_set_root(template_mut_doc, new_val);
    }
}
std::string timestampToString(time_t seconds) {
    struct tm* timeinfo = localtime(&seconds);
    if (timeinfo == nullptr) {
        return "Invalid timestamp";
    }
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}
void format_time_fields(yyjson_mut_doc *template_mut_doc) {

    yyjson_mut_val *root = yyjson_mut_doc_get_root(template_mut_doc);
    if (yyjson_mut_is_obj(root)) {
        // 处理 time 字段，转换时间戳为日期字符串
        yyjson_mut_val *time_val = yyjson_mut_obj_get(root, "time");
        if (!yyjson_mut_is_num(time_val))
        {
            return;
        }
        uint64_t timestamp = yyjson_mut_get_uint(time_val);
        // std::cout << timestamp<<std::endl;
        std::string formatted_time = timestampToString(timestamp/1000);
        // 替换为格式化的时间字符串
        yyjson_mut_val *formatted_val = yyjson_mut_strcpy(template_mut_doc, formatted_time.c_str());
        // std::cout<<yyjson_mut_get_str(formatted_val)<<std::endl;
        yyjson_mut_ptr_replace(root,"/time",formatted_val);
    }
}

char *transform(char *original_str) {
    yyjson_doc *original_doc = yyjson_read(original_str, strlen(original_str), 0);
    yyjson_mut_doc *original_mut_doc = yyjson_doc_mut_copy(original_doc, nullptr);
    yyjson_doc *template_doc = yyjson_read(TEMPLATE_STR, strlen(TEMPLATE_STR), 0);
    yyjson_mut_doc *template_mut_doc = yyjson_doc_mut_copy(template_doc, nullptr);
    yyjson_doc_free(original_doc);
    yyjson_doc_free(template_doc);

    start_process(original_mut_doc, template_mut_doc);
    format_time_fields(template_mut_doc);
    char *result = yyjson_mut_write(template_mut_doc, 0, nullptr);

    yyjson_mut_doc_free(original_mut_doc);
    yyjson_mut_doc_free(template_mut_doc);

    return result;
}

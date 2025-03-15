//
// Created by root on 3/7/25.
//

#ifndef JSON_ALGORITHM_H
#define JSON_ALGORITHM_H
#ifdef __cplusplus
extern "C" {
#endif
#define TEMPLATE_STR "{\"gateid\":\"/node\",\"time\":\"/timestamp\",\"source\":\"da/db\",\"meter\":[{\"id\":\"/group\",\"status\":\"1\",\"name\":\"/group\",\"values\":\"/values\"}]}"
char* transform(char* original_str);
#ifdef __cplusplus
}
#endif

#endif //JSON_ALGORITHM_H

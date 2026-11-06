#pragma once
#include <cstddef>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct cJSON { char* valuestring; int valueint; double valuedouble; } cJSON;
cJSON* cJSON_ParseWithLength(const char*, size_t);
cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON*,const char*);
int cJSON_IsString(const cJSON*);
int cJSON_IsNumber(const cJSON*);
int cJSON_IsBool(const cJSON*);
int cJSON_IsFalse(const cJSON*);
void cJSON_Delete(cJSON*);
#ifdef __cplusplus
}
#endif

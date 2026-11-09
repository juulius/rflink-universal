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
cJSON* cJSON_CreateObject(void);
cJSON* cJSON_AddStringToObject(cJSON*,const char*,const char*);
cJSON* cJSON_AddNumberToObject(cJSON*,const char*,double);
cJSON* cJSON_AddBoolToObject(cJSON*,const char*,int);
char* cJSON_PrintUnformatted(const cJSON*);
void cJSON_free(void*);
#ifdef __cplusplus
}
#endif

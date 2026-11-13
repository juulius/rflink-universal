#pragma once
#include <cstdint>
#include <cstddef>
#include "esp_err.h"
using httpd_handle_t=void*;
struct httpd_req_t{int content_len=0;const char* uri="";int method=0;};
using httpd_err_code_t=int;
using httpd_err_handler_func_t=esp_err_t (*)(httpd_req_t*,httpd_err_code_t);
struct httpd_config_t{unsigned stack_size=0;int max_uri_handlers=0;int recv_wait_timeout=0;};
#define HTTPD_DEFAULT_CONFIG() httpd_config_t{}
struct httpd_uri_t{const char* uri=nullptr;int method=0;esp_err_t (*handler)(httpd_req_t*)=nullptr;void* user_ctx=nullptr;};
constexpr int HTTP_GET=0,HTTP_POST=1;
constexpr int HTTPD_403_FORBIDDEN=403,HTTPD_400_BAD_REQUEST=400,HTTPD_404_NOT_FOUND=404,HTTPD_500_INTERNAL_SERVER_ERROR=500;
constexpr int HTTPD_SOCK_ERR_TIMEOUT=-2;
extern "C" esp_err_t httpd_resp_set_type(httpd_req_t*,const char*);
extern "C" esp_err_t httpd_resp_set_status(httpd_req_t*,const char*);
extern "C" esp_err_t httpd_resp_set_hdr(httpd_req_t*,const char*,const char*);
extern "C" esp_err_t httpd_resp_sendstr(httpd_req_t*,const char*);
extern "C" esp_err_t httpd_resp_send_err(httpd_req_t*,int,const char*);
extern "C" esp_err_t httpd_req_get_hdr_value_str(httpd_req_t*,const char*,char*,size_t);
extern "C" int httpd_req_recv(httpd_req_t*,char*,size_t);
extern "C" esp_err_t httpd_start(httpd_handle_t*,const httpd_config_t*);
extern "C" esp_err_t httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t*);
extern "C" esp_err_t httpd_register_err_handler(httpd_handle_t,httpd_err_code_t,httpd_err_handler_func_t);
extern "C" esp_err_t httpd_stop(httpd_handle_t);
extern "C" esp_err_t httpd_req_get_url_query_str(httpd_req_t*,char*,size_t);
extern "C" esp_err_t httpd_query_key_value(const char*,const char*,char*,size_t);
// HTTP streaming is stubbed for host syntax audit; ESP-IDF implements it.
extern "C" esp_err_t httpd_resp_send_chunk(httpd_req_t*,const char*,size_t);

// Async HTTP handlers allow long-running exports without blocking /api/status.
extern "C" esp_err_t httpd_req_async_handler_begin(httpd_req_t*,httpd_req_t**);
extern "C" esp_err_t httpd_req_async_handler_complete(httpd_req_t*);

#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <cstring>
#define HTTP_GET 0
#define HTTP_POST 1
class StringRef { public: StringRef(const char* p="", size_t n=0):p_(p),n_(n){} const char* c_str() const{return p_;} size_t size() const{return n_;} bool operator==(const std::string& s) const{return s.size()==n_ && std::string(p_,n_)==s;} private: const char* p_; size_t n_; };
class AsyncWebServerResponse { public: void addHeader(const char*,const char*){}; };
class AsyncWebServerRequest { public: static constexpr size_t URL_BUF_SIZE=256; int method() const{return HTTP_GET;} template<size_t N> StringRef url_to(char (&b)[N]) const { const char *u="/rflink"; std::strncpy(b,u,N); return StringRef(b,std::strlen(b)); } std::string url() const{return "/rflink";} std::string arg(const char*){return {};} bool hasArg(const char*){return false;} AsyncWebServerResponse* beginResponse(int,const char*,const std::string&){return new AsyncWebServerResponse();} AsyncWebServerResponse* beginResponse(int,const char*,const uint8_t*,size_t){return new AsyncWebServerResponse();} void send(AsyncWebServerResponse*){}; void redirect(const std::string&){}; };
class AsyncWebHandler { public: virtual ~AsyncWebHandler()=default; virtual bool canHandle(AsyncWebServerRequest*) const{return false;} virtual void handleRequest(AsyncWebServerRequest*){}; };
namespace esphome { namespace web_server_base { class WebServerBase { public: void add_handler(AsyncWebHandler*){}; void init(){}; }; extern WebServerBase *global_web_server_base; }}

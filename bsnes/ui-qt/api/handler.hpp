#pragma once
#include <pistache/endpoint.h>

class RestHandler : public Net::Http::Handler {
  HTTP_PROTOTYPE(RestHandler)

  public:
    void onRequest(const Net::Http::Request& req, Net::Http::ResponseWriter response);
  private:
    static std::string trimString(std::string str);
    static void handleGet(const Net::Http::Request& req, Net::Http::ResponseWriter& response);
    static void handlePatch(const Net::Http::Request& req, Net::Http::ResponseWriter& response);
};

#pragma once
#include <thread>
#include "server_http.hpp"

class RestHandler {
  public:
  RestHandler(int port);
  void start();
  void stop();
  private:
  static std::string trimString(std::string str);

  static void handleGet(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request);
  static void handlePatch(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request);

  static void ok(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    const std::string& content);
  static void badRequest(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    const std::string& content);
  static void notFound(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    const std::string& content);

  static void respond(
    std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
    int status_code,
    const std::string& status_text,
    const std::string& content);

  SimpleWeb::Server<SimpleWeb::HTTP> _server;
  std::thread* _thread;
};

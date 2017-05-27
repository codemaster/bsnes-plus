#include <chrono>
#include "handler.hpp"

static const std::regex posRegex("position=([0-9a-fA-F]+)");
static const std::regex countRegex("count=([0-9]+)");

RestHandler::RestHandler(int port) : _thread(nullptr) {
  _server.config.port = port;
  _server.default_resource["GET"] = handleGet;
  _server.default_resource["PATCH"] = handlePatch;
}

std::string RestHandler::trimString(std::string str) {
  auto isspace = []( char ch ) { return std::isspace<char>( ch, std::locale::classic() ); };
  str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
  return std::move(str);
}

void RestHandler::start() {
  if (nullptr != _thread) {
    return;
  }

  _thread = new std::thread([this](){
      _server.start();
  });

  // Have to wait for 1ms here for the REST server to kick off
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void RestHandler::stop() {
  _server.stop();

  if (nullptr == _thread) {
      return;
  }

  _thread->join();
  delete _thread;
  _thread = nullptr;
}

void RestHandler::handleGet(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {

  // Ensure we have a game loaded
  if (!SNES::cartridge.loaded()) {
    notFound(response, "No game loaded");
    return;
  }

  // Obtain the query parameters
  auto query = request->path;

  // Obtain the position if it's available
  std::smatch posMatches;
  if(!std::regex_search(query, posMatches, posRegex) || posMatches.size() < 1) {
    notFound(response, "No position provided.");
    return;
  }

  // Convert the position from hex string to a number
  // This is guaranteed to be 0+ because this is an unsigned int
  unsigned int position = std::stoul(posMatches[1], nullptr, 16);

  // Obtain the count if available - otherwise default to 1
  std::smatch countMatches;
  unsigned int count = 1;
  if(std::regex_search(query, countMatches, countRegex) && countMatches.size() > 0) {
      count = std::stoul(countMatches[1]);
  }

  // Validate the count
  if (count == 0) {
      badRequest(response, "Invalid count; must be greater than 0.");
      return;
  }

  // Read all of the addresses needed and add it to the string stream, separated by spaces
  std::stringstream sstream;
  SNES::debugger.bus_access = true;
  for (int i = 0; i < count; ++i) {
    auto byte = SNES::debugger.read(SNES::Debugger::MemorySource::CPUBus, position + i);
    sstream << std::hex << byte;
    sstream << ' ';
  }
  SNES::debugger.bus_access = false;

  ok(response, sstream.str());
}

void RestHandler::handlePatch(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {

  // Ensure we have a game loaded
  if (!SNES::cartridge.loaded()) {
    notFound(response, "No game loaded");
    return;
  }

  // Obtain the query parameters
  auto query = request->path;

  // Obtain the position if it's available
  std::smatch posMatches;
  if(!std::regex_search(query, posMatches, posRegex) || posMatches.size() < 1) {
    notFound(response, "No position provided.");
    return;
  }

  // Convert the position from hex string to a number
  // This is guaranteed to be 0+ because this is an unsigned int
  unsigned int position = std::stoul(posMatches[1], nullptr, 16);

  // Ensure we are provided a body that is properly structured
  auto body = trimString(request->content.string());
  if(body.length() < 2 || body.length() % 2 != 0) {
    badRequest(response, "Must provide hexadecimal data on a per-byte basis");
    return;
  }

  // Loop through the provided body and write the data to the memory
  std::stringstream stream;
  SNES::debugger.bus_access = true;
  for (int i = 0; i < body.length(); i = i + 2) {
    // Grab 2 numbers at a time
    auto numStr = body.substr(i, 2);
    // Convert it from hex to an actual number
    auto num = std::stoul(numStr, nullptr, 16);
    // Write to memory
    SNES::debugger.write(SNES::Debugger::MemorySource::CPUBus, position++, num);
  }
  SNES::debugger.bus_access = false;

  ok(response, "");
}

void RestHandler::ok(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  const std::string& content) {
    respond(response, 200, "OK", content);
}

void RestHandler::badRequest(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  const std::string& content) {
    respond(response, 400, "Bad Request", content);
}

void RestHandler::notFound(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  const std::string& content) {
    respond(response, 404, "Not Found", content);
}

void RestHandler::respond(
  std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response,
  int status_code,
  const std::string& status_text,
  const std::string& content) {
  *response << "HTTP/1.1 " << status_code << " " << status_text <<
    "\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
}

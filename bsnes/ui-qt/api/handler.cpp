#include "handler.hpp"

std::string RestHandler::trimString(std::string str) {
    auto isspace = []( char ch ) { return std::isspace<char>( ch, std::locale::classic() ); };
    str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
    return std::move(str);
}

void RestHandler::onRequest(const Net::Http::Request& req, Net::Http::ResponseWriter response) {
    // Ensure we have a game loaded
    if (!SNES::cartridge.loaded()) {
        response.send(Net::Http::Code::Not_Found, "No game loaded");
        return;
    }

    // Ignore all but GET and PATCH requests
    auto method = req.method();

    switch (method)
    {
        case Net::Http::Method::Get:
            handleGet(req, response);
            break;
        case Net::Http::Method::Patch:
            handlePatch(req, response);
            break;
        default:
            response.send(Net::Http::Code::Bad_Request, "Invalid HTTP verb: this only handles GET and PATCH");
    }
}

void RestHandler::handleGet(const Net::Http::Request& req, Net::Http::ResponseWriter& response) {
    // Ensure we were provided a position and a count
    auto query = req.query();
    if (!query.has("position")) {
        response.send(Net::Http::Code::Bad_Request, "No position provided");
        return;
    }
    if (!query.has("count")) {
        response.send(Net::Http::Code::Bad_Request, "No count provided");
        return;
    }

    // Only if we can obtain position and count, kick off the rest of the logic
    optionally_do(query.get("position"), [&response, &query](const std::string& pos) {
        optionally_do(query.get("count"), [&response, &pos](const std::string& count) {
            auto posNum = (unsigned int) std::stoul(pos, nullptr, 16);
            auto countNum = (unsigned int) std::stoul(count, nullptr, 10);

            // Validate the provided parameters
            if (posNum < 0) {
                response.send(Net::Http::Code::Bad_Request, "Invalid position; must be 0 or more.");
                return;
            }

            if (countNum <= 0) {
                response.send(Net::Http::Code::Bad_Request, "Invalid count; must be greater than 0.");
                return;
            }

            // Read all of the addresses needed and add it to the string stream, separated by spaces
            std::stringstream sstream;
            SNES::debugger.bus_access = true;
            for (int i = 0; i < countNum; ++i) {
                auto byte = SNES::debugger.read(SNES::Debugger::MemorySource::CPUBus, posNum + i);
                sstream << std::hex << byte;
                sstream << ' ';
            }
            SNES::debugger.bus_access = false;

            response.send(Net::Http::Code::Ok, sstream.str());
        });
    });
}

void RestHandler::handlePatch(const Net::Http::Request& req, Net::Http::ResponseWriter& response) {
    // Ensure we are provided a position
    auto query = req.query();
    if (!query.has("position")) {
        response.send(Net::Http::Code::Bad_Request, "No position provided");
        return;
    }

    // Ensure we are provided a body that is properly structured
    auto body = trimString(req.body());
    if(body.length() < 2 || body.length() % 2 != 0) {
        response.send(Net::Http::Code::Bad_Request, "Must provide hexadecimal data on a per-byte basis");
        return;
    }

    // Only if we are given a position, kick off the rest of the logic
    optionally_do(query.get("position"), [&response, &body](const std::string& pos) {
        auto posNum = std::stoul(pos, nullptr, 16);

        // Validate the position
        if (posNum < 0) {
            response.send(Net::Http::Code::Bad_Request, "Invalid position; must be 0 or more.");
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
            SNES::debugger.write(SNES::Debugger::MemorySource::CPUBus, posNum++, num);
        }
        SNES::debugger.bus_access = false;

        response.send(Net::Http::Code::Ok);
    });
}

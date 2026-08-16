#pragma once

#include <string>
#include "httplib.h"
#include "json.hpp"
#include "manager/InterviewStreamManager.hpp"

class InterviewStreamController {
public:
    explicit InterviewStreamController(InterviewStreamManager& manager);

    void streamInterview(const httplib::Request& req, httplib::Response& res);

private:
    InterviewStreamRequest parseStreamRequest(const nlohmann::json& body) const;
    static std::string serializeSseEvent(const std::string& eventName, const std::string& data);

    InterviewStreamManager& m_manager;
};

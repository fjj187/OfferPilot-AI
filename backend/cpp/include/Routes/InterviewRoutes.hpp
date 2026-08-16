#pragma once

#include <string>

#include "http_server.hpp"
#include "Controller/interview_controller.hpp"
#include "Controller/InterviewStreamController.hpp"

class InterviewRoutes {
public:
    InterviewRoutes(HttpServer& httpServer,
                    InterviewController& controller,
                    InterviewStreamController& streamController);

    void registerRoutes();

private:
    HttpServer& m_httpServer;
    InterviewController& m_controller;
    InterviewStreamController& m_streamController;
};
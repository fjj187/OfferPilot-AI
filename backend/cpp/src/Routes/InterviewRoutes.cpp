#include "Routes/InterviewRoutes.hpp"

InterviewRoutes::InterviewRoutes(HttpServer& httpServer,
                                 InterviewController& controller,
                                 InterviewStreamController& streamController)
    : m_httpServer(httpServer),
      m_controller(controller),
      m_streamController(streamController) {}

void InterviewRoutes::registerRoutes() {
    m_httpServer.post("/api/interview/stream",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_streamController.streamInterview(req, res);
        });

    m_httpServer.post("/api/interview/reports/generate",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.generateReport(req, res);
        });

    m_httpServer.get("/api/interview/reports",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.listReports(req, res);
        });

    m_httpServer.get("/api/interview/reports/:sessionId",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.getReport(req, res);
        });

    m_httpServer.get("/api/interview/sessions",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.listSessions(req, res);
        });

    m_httpServer.get("/api/interview/sessions/:sessionId/:threadId",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.getSession(req, res);
        });

    m_httpServer.post("/api/interview/history/clear",
        [this](const httplib::Request& req, httplib::Response& res) {
            m_controller.clearHistory(req, res);
        });
}
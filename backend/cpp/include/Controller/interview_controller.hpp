#pragma once
#include <string>

#include "httplib.h"
#include "json.hpp"
#include "services/InterviewService.hpp"
#include "services/ReportService.hpp"

class InterviewController {
public:
    explicit InterviewController(InterviewService& service,ReportService& reportService) : m_service(service),m_reportService(reportService) {};
    void generateReport(const httplib::Request& req, httplib::Response& res);
    void getReport(const httplib::Request& req, httplib::Response& res);
    void listReports(const httplib::Request& req, httplib::Response& res);
    void createSession(const httplib::Request& req, httplib::Response& res);
    void sendMessage(const httplib::Request& req, httplib::Response& res);
    void listSessions(const httplib::Request& req, httplib::Response& res);
    void getSession(const httplib::Request& req, httplib::Response& res);
    void clearHistory(const httplib::Request& req, httplib::Response& res);
private:
    InterviewStreamRequest parseStreamRequest(const nlohmann::json& body) const;
    static std::string serializeSseEvent(const std::string &eventName,const std::string &data);
    InterviewService& m_service;
    ReportService& m_reportService;
};

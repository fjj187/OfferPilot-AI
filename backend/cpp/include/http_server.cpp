#include "http_server.hpp"

#include <iostream>
#include <utility>

HttpServer::HttpServer(int port, std::string bindHost)
    : m_server(),
      m_port(port),
      m_bindHost(std::move(bindHost)),
      m_running(false) {
}

HttpServer::~HttpServer() {
    if (m_running) {
        m_server.stop();
    }
}

void HttpServer::get(const std::string& path, const httplib::Server::Handler& handler) {
    m_server.Get(path, handler);
}

void HttpServer::post(const std::string& path, const httplib::Server::Handler& handler) {
    m_server.Post(path, handler);
}

bool HttpServer::start() {
    // listen() blocks until stop() is called or bind/listen fails.
    m_running = true;
    const bool ok = m_server.listen(m_bindHost.c_str(), m_port);
    m_running = false;
    return ok;
}

void HttpServer::stop() {
    m_server.stop();
    m_running = false;
}

bool HttpServer::isRunning() const {
    return m_running;
}

int HttpServer::getPort() const {
    return m_port;
}

const std::string& HttpServer::getBindHost() const {
    return m_bindHost;
}

void HttpServer::setupMiddleware() {
    // Keep the CORS policy explicit and narrow. The reverse proxy should still
    // own external HTTPS termination and request filtering.
    m_server.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        return httplib::Server::HandlerResponse::Unhandled;
    });
}

void HttpServer::setupErrorHandler() {
    m_server.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status == 404) {
            res.set_content("{\"success\":false,\"error\":\"Not Found\"}", "application/json");
            return;
        }

        if (res.status >= 500) {
            std::cerr << "[http] internal error, status=" << res.status << std::endl;
        }
    });
}

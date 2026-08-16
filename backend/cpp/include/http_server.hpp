#pragma once

#include <string>

#include "httplib.h"

// Lightweight wrapper around httplib::Server.
// The wrapper keeps bind host and lifecycle control explicit for deployment.
class HttpServer {
public:
    HttpServer(int port, std::string bindHost = "127.0.0.1");
    ~HttpServer();

    // Start the HTTP server. Returns false when bind/listen fails.
    bool start();

    // Stop the HTTP server.
    void stop();

    // Check whether the server is running.
    bool isRunning() const;

    // Get the configured listen port.
    int getPort() const;

    // Get the configured bind host.
    const std::string& getBindHost() const;

    // Register GET route.
    void get(const std::string& path, const httplib::Server::Handler& handler);

    // Register POST route.
    void post(const std::string& path, const httplib::Server::Handler& handler);

    // Configure middleware.
    void setupMiddleware();

    // Configure centralized error handling.
    void setupErrorHandler();

private:
    httplib::Server m_server;
    int m_port;
    std::string m_bindHost;
    bool m_running;
};

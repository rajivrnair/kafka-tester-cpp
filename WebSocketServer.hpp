// WebSocketServer.hpp
#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <set>

using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer();
    void run();
    void stop();
    bool broadcast(const std::string& message);

private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using connection_hdl = websocketpp::connection_hdl;

    void onMessage(connection_hdl hdl, Server::message_ptr msg);
    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);
    bool validateMessage(const json& j);

    Server server_;
    std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
    uint16_t port_;
};
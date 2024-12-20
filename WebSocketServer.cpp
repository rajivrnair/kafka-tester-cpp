// WebSocketServer.cpp
#include "WebSocketServer.hpp"

WebSocketServer::WebSocketServer() : port_(9002) {
    server_.set_access_channels(websocketpp::log::alevel::all);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    
    server_.init_asio();

    server_.set_message_handler(
        std::bind(&WebSocketServer::onMessage, this, 
                 std::placeholders::_1, std::placeholders::_2));
    
    server_.set_open_handler(
        std::bind(&WebSocketServer::onOpen, this, std::placeholders::_1));
    
    server_.set_close_handler(
        std::bind(&WebSocketServer::onClose, this, std::placeholders::_1));
}

void WebSocketServer::run() {
    server_.listen(port_);
    server_.start_accept();
    std::cout << "WS: WebSocket server listening on port " << port_ << std::endl;
    server_.run();
}

void WebSocketServer::stop() {
    server_.stop();
}

bool WebSocketServer::validateMessage(const json& j) {
    try {
        if (!j.contains("id") || !j.contains("content") || !j.contains("timestamp")) {
            std::cerr << "Missing required fields" << std::endl;
            return false;
        }

        if (!j["id"].is_string() || !j["content"].is_string() || !j["timestamp"].is_number()) {
            std::cerr << "Invalid field types" << std::endl;
            return false;
        }

        if (j["id"].get<std::string>().empty()) {
            std::cerr << "ID cannot be empty" << std::endl;
            return false;
        }

        if (j["timestamp"].get<int64_t>() <= 0) {
            std::cerr << "Invalid timestamp" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Validation error: " << e.what() << std::endl;
        return false;
    }
}

bool WebSocketServer::broadcast(const std::string& message) {
    try {
        json j = json::parse(message);
        
        if (!validateMessage(j)) {
            std::cerr << "Message validation failed" << std::endl;
            return false;
        }

        for (auto& hdl : connections_) {
            server_.send(hdl, message, websocketpp::frame::opcode::text);
        }
        
        std::cout << "\nWS: Broadcasted message: " << message << "\n" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "\nWS: Broadcast error: " << e.what() << "\n" << std::endl;
        return false;
    }
}

void WebSocketServer::onMessage(connection_hdl hdl, Server::message_ptr msg) {
    try {
        json j = json::parse(msg->get_payload());
        
        if (validateMessage(j)) {
            std::cout << "Received valid message: " << j.dump() << std::endl;
            server_.send(hdl, msg->get_payload(), msg->get_opcode());
        } else {
            std::cout << "Received invalid message" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Message handling error: " << e.what() << std::endl;
    }
}

void WebSocketServer::onOpen(connection_hdl hdl) {
    connections_.insert(hdl);
    std::cout << "Client connected" << std::endl;
}

void WebSocketServer::onClose(connection_hdl hdl) {
    connections_.erase(hdl);
    std::cout << "Client disconnected" << std::endl;
}
#ifndef __VARIABLESYNCSERVICE_HPP__
#define __VARIABLESYNCSERVICE_HPP__


#include "asio.hpp"
#include <iostream>
#include <string>

using asio::ip::tcp;

// 连接信息
struct NetworkSession {
    asio::io_context io;
    tcp::socket socket;
    tcp::resolver resolver;
    asio::error_code ec;

    NetworkSession() : socket(io), resolver(io) {}
};

// 1. 建立网络连接
bool connect_to_server(NetworkSession& session, const std::string& host, const std::string& port) {
    auto endpoints = session.resolver.resolve(host, port, session.ec);
    if (session.ec) {
        std::cerr << "Resolve failed: " << session.ec.message() << "\n";
        return false;
    }

    asio::connect(session.socket, endpoints, session.ec);
    if (session.ec) {
        std::cerr << "Connect failed: " << session.ec.message() << "\n";
        return false;
    }

    // std::cout << "Connected to server\n";
    return true;
}

// 2. 发送信息并接收回显
std::string send_message(NetworkSession& session, const std::string& msg) {
    asio::write(session.socket, asio::buffer(msg), session.ec);
    if (session.ec) {
        std::cerr << "Send failed: " << session.ec.message() << "\n";
        return "";
    }

    char buf[512];
    size_t n = session.socket.read_some(asio::buffer(buf), session.ec);
    if (session.ec) {
        std::cerr << "Receive failed: " << session.ec.message() << "\n";
        return "";
    }
    return std::string(buf, n);
}

// 3. 关闭连接
void close_connection(NetworkSession& session) {
    session.socket.close(session.ec);
    if (session.ec) {
        std::cerr << "Close failed: " << session.ec.message() << "\n";
    } else {
        // std::cout << "Connection closed\n";
    }
}

#endif
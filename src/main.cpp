#include <iomanip>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/websocket.hpp>
#include <dotenv.h>

#include <iostream>
#include <filesystem>

#include "WebSocketClient.h"

using TransportProject::WebsocketClient;

using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
namespace websocket = boost::beast::websocket;

void Log(const std::string& where, const error_code &ec)
{
    std::cerr << "[" << std::setw(20) << where << "] "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "")
              << std::endl;
}


int main() {
    std::cout << "Current Working Directory: " << std::filesystem::current_path() << std::endl;
    dotenv::init();
    const char* port_env = std::getenv("WEBSOCKET_SERVER_PORT");
    const char* url_env = std::getenv("WEBSOCKET_SERVER_URL");
    if (!port_env || !url_env) {
        std::cerr << "Error: WEBSOCKET_SERVER_PORT or WEBSOCKET_SERVER_URL not set in .env file." << std::endl;
        return 1;
    }


    const std::string url {url_env};
    const std::string port {port_env};
    const std::string endpoint {"/echo"};
    const std::string message {"Hello, WebSocket!"};

    std::cout << "[ Main Thread : " << std::setw(14) << std::this_thread::get_id() << " ]" << std::endl;

    boost::asio::io_context io_context {};

    // Always start with an I/O context object.
    boost::asio::io_context ioc {};

    // The class under test
    WebsocketClient client {url, endpoint, port, ioc};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool messageMatches {false};
    bool disconnected {false};

    // Our own callbacks
    auto onSend = [&messageSent](auto ec) {
        messageSent = !ec;
    };
    auto onConnect = [&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    };
    auto onClose = [&disconnected](auto ec) {
        disconnected = !ec;
    };
    auto onReceive = [&client, &onClose, &messageReceived, &messageMatches, &message](auto ec, const auto& received) {
        messageReceived = !ec;
        messageMatches = message == received;
        client.Close(onClose);
    };
    auto onDisconnect = [&disconnected](auto ec) {
        disconnected = !ec;
    };

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive, onDisconnect);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    bool ok {
        connected &&
        messageSent &&
        messageReceived &&
        messageMatches &&
        disconnected
    };
    if (ok) {
        std::cout << "Websocket Work done." << std::endl;
        return 0;
    } else {
        std::cerr << "Test failed" << std::endl;
        return 1;
    }
}
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/websocket.hpp>
#include <dotenv.h>

#include <iostream>
#include <filesystem>

using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
namespace websocket = boost::beast::websocket;

void Log(const std::string& where, error_code ec)
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
    const std::string message {"Hello, Socket!"};

    std::cout << "[ Main Thread : " << std::setw(14) << std::this_thread::get_id() << " ]" << std::endl;

    boost::asio::io_context io_context {};

    tcp::socket socket {boost::asio::make_strand(io_context)};

    boost::system::error_code ec {};

    tcp::resolver resolver {io_context};
    const auto endpoints {resolver.resolve(url, port, ec)};

    if (ec) {
        Log("resolver", ec);
        return -1;
    }

    socket.connect(*endpoints.begin(), ec);
    if (ec) {
        Log("socket.connect", ec);
        return -1;
    }

    // perform WebSocket handshake
    websocket::stream<tcp::socket> ws {std::move(socket)};

    auto OnReceiveMessage = [](const error_code& err, boost::beast::flat_buffer receive_buffer) {
        if (err) {
            Log("OnReceiveMessage", err);
            return;
        }

        std::cout << "Received echoed message: "
            << boost::beast::make_printable(receive_buffer.data())
            << std::endl;

    };

    auto OnSendMessage = [&ws, &OnReceiveMessage](const error_code& ec, std::size_t bytes_written) {
        if (ec) {
            Log("OnSendMessage", ec);
            return;
        }

        std::cout << "Message sent successfully (" << bytes_written << " bytes)" << std::endl;

        // Read the echoed message back.
        auto receive_buffer = std::make_shared<boost::beast::flat_buffer>();
        ws.async_read(*receive_buffer, [receive_buffer, OnReceiveMessage](const error_code& ec, std::size_t bytes_read) {
            OnReceiveMessage(ec,*receive_buffer);
        });
    };

    auto OnHandshake = [&ws, &message, &OnSendMessage, &url, &port](const error_code& ec_handshake) {
        if (ec_handshake) {
            Log("OnHandshake", ec_handshake);
            return;
        }

        std::cout << "WebSocket Sending message to: " << url <<":" << port << std::endl;

        const boost::asio::const_buffer send_buffer {message.c_str(), message.size()};
        ws.async_write(send_buffer, OnSendMessage);
    };

    // 1. handshake request is sent asynchronously
    // 2. when handshake response is received, we send a Hello message asynchronously
    // 3. when echoed message is received, we print it to console
    // capture ws by reference, not by value
    ws.async_handshake(url, "/echo", OnHandshake);

    io_context.run();
    return 0;
}
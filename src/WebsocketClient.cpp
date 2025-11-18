//
// Created by Ajinkya Borade on 14/11/25.
//

#include "WebsocketClient.h"

#include <utility>

TransportProject::WebsocketClient::WebsocketClient(
    const std::string &url, const std::string &endpoint, const std::string &port, boost::asio::io_context &ioc)
    : resolver_{boost::asio::make_strand(ioc)},
      ws_{boost::asio::make_strand(ioc)},
      url_{url}, endpoint_{endpoint}, port_{port}
{
}

TransportProject::WebsocketClient::~WebsocketClient() = default;

void TransportProject::WebsocketClient::Connect(
    std::function<void(error_code)> onConnect, std::function<void(error_code, std::string &&)> onMessage, std::function<void(error_code)> onDisconnect)
{
    onConnect_ = std::move(onConnect);
    onMessage_ = std::move(onMessage);
    onDisconnect_ = std::move(onDisconnect);

    resolver_.async_resolve(url_, port_, [this](const error_code& ec, const auto& resolveIt) {
        this->OnResolve(ec, resolveIt);
    });
}

void TransportProject::WebsocketClient::Send(const std::string &message, std::function<void(error_code)> onSend) {
    this->ws_.async_write(
    boost::asio::buffer(message),
    // onSend passed here as copy to keep the it alive until the lambda is executed
    [onSend](const error_code &ec, const std::size_t bytes_transferred) {
        Log("WebsocketClient::Send", ec);
        if (onSend) {
            onSend(ec);
        }
    });
}

void TransportProject::WebsocketClient::Close(const std::function<void(error_code)>& onClose) {
    closed_ = true;
    ws_.async_close(
        boost::beast::websocket::close_code::none,
        [onClose](const error_code &ec) {
            if (onClose) {
                onClose(ec);
            }
        }
    );
}

void TransportProject::WebsocketClient::OnResolve(const boost::system::error_code &ec, const boost::asio::ip::tcp::resolver::iterator &resolverIt) {
    if (ec) {
        Log("WebsocketClient::OnResolve", ec);
        this->onConnect_(ec);
        return;
    }

    ws_.next_layer().expires_after(std::chrono::seconds(5));

    // Connect to the TCP socket.
    // Instead of constructing the socket and the ws objects separately, the
    // socket is now embedded in ws_, and we access it through next_layer().
    ws_.next_layer().async_connect(*resolverIt, [this](const error_code& err_connect) {
        this->OnConnect(err_connect);
    });
}

void TransportProject::WebsocketClient::OnConnect(const boost::system::error_code &ec) {

    if (ec) {
        Log("WebsocketClient::OnConnect", ec);
        return;
    }

    // Now that the TCP socket is connected, we can reset the timeout to
    // whatever Boost.Beast recommends.
    ws_.next_layer().expires_never();
    ws_.set_option(
        boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::client
        )
    );

    // Handshake, if successful we are connected.
    ws_.async_handshake(url_, endpoint_, [this](const error_code& err_handshake) {
        this->OnHandshake(err_handshake);
    });
}

void TransportProject::WebsocketClient::OnHandshake(const boost::system::error_code &ec) {
    closed_ = false;
    if (ec) {
        Log("WebsocketClient::OnHandshake", ec);
        return;
    }

    // use text form
    ws_.text(true);

    ListenToIncomingMessage(ec);
    // Notify that we are connected
    if (onConnect_) onConnect_(ec);
}

void TransportProject::WebsocketClient::ListenToIncomingMessage(const boost::system::error_code &ec) {
    Log("WebsocketClient::ListenToIncomingMessage", ec);
    // Stop processing messages if the connection has been aborted.
    if (ec == boost::asio::error::operation_aborted) {
        // We check the closed_ flag to avoid notifying the user twice.
        if (onDisconnect_ && !closed_) {
            onDisconnect_(ec);
        }
        return;
    }

    // Parse the message and forward it to the user callback.
    // Note: This call is synchronous and will block the WebSocket strand.
    ws_.async_read(this->rBuffer_, [this](const error_code &err_read, const size_t& readBytes) {
        this->OnRead(err_read, readBytes);
    });
}

void TransportProject::WebsocketClient::OnRead(const boost::system::error_code &ec, const size_t& nBytes) {
    Log("WebsocketClient::OnRead", ec);
    if (ec) {
        return;
    }

    std::string message {
        boost::beast::buffers_to_string(this->rBuffer_.data())
    };
    rBuffer_.consume(nBytes);
    std::cout << "[ WebsocketClient::OnRead ] Received (" << message << ") echo!" << std::endl;
    if (onMessage_) {
        onMessage_(ec, std::move(message));
    }

    // continue listening for more messages
    ListenToIncomingMessage(ec);
}

// base_wss_client.hpp
#pragma once

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp> // ✅ Needed for SSL stream teardown
#include <nlohmann/json.hpp>
#include <zmq.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace websocket = boost::beast::websocket;
using tcp = net::ip::tcp;
using json = nlohmann::json;
using net::awaitable;
using net::use_awaitable;

class BaseWssClient {
protected:
    std::string host_, port_, endpoint_;
    ssl::context& ssl_ctx_;
    net::io_context& ioc_;
    websocket::stream<ssl::stream<tcp::socket>> ws_;
    zmq::socket_t publisher_;
    std::string zmq_bind_address_;

    virtual void handle_message(const std::string& raw_msg) = 0;
    virtual void subscribe_channels() = 0;

public:
    BaseWssClient(const std::string& host, const std::string& port,
                  const std::string& endpoint,
                  const std::string& zmq_bind_address,
                  zmq::context_t& zmq_ctx,
                  net::io_context& ioc,
                  ssl::context& ssl_ctx)
        : host_(host), port_(port), endpoint_(endpoint),
          ssl_ctx_(ssl_ctx), ioc_(ioc),
          ws_(ioc, ssl_ctx),
          publisher_(zmq_ctx, zmq::socket_type::pub),
          zmq_bind_address_(zmq_bind_address) {
        ssl_ctx_.set_default_verify_paths();
        publisher_.bind(zmq_bind_address_);
        std::cout << "ZeroMQ Publisher bound at " << zmq_bind_address_ << "\n";
    }

    virtual awaitable<void> run() {
        try {
            tcp::resolver resolver(co_await net::this_coro::executor);
            auto endpoints = co_await resolver.async_resolve(host_, port_, use_awaitable);
            co_await net::async_connect(ws_.next_layer().next_layer(), endpoints, use_awaitable);

            if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
                throw std::runtime_error("SSL host name setting failed");
            }

            co_await ws_.next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
            co_await ws_.async_handshake(host_ + ":" + port_, endpoint_, use_awaitable);

            std::cout << "Connected to " << host_ << " WebSocket" << std::endl;

            subscribe_channels();
            boost::beast::flat_buffer buffer;

            while (true) {
                buffer.clear();
                co_await ws_.async_read(buffer, use_awaitable);
                std::string msg = boost::beast::buffers_to_string(buffer.data());
                handle_message(msg);
            }

        } catch (const std::exception& e) {
            std::cerr << "[BaseWssClient Error] " << e.what() << "\n";
        }
        co_return;
    }

    void close() {
        ws_.close(websocket::close_code::normal);
    }

protected:
    void publish(const std::string& topic, const json& payload) {
        zmq::message_t topic_msg(topic.data(), topic.size());
        zmq::message_t data_msg(payload.dump());
        publisher_.send(topic_msg, zmq::send_flags::sndmore);
        publisher_.send(data_msg, zmq::send_flags::none);
        std::cout << "[ZMQ Sent] " << payload.dump() << std::endl;
    }
};

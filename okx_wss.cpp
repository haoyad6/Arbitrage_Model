#include "okx_wss.hpp"
#include <iostream>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <openssl/err.h>



namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;
using net::awaitable;
using net::use_awaitable;

OkxWss::OkxWss(const std::string& host, const std::string& port,
               const std::string& endpoint,
               zmq::context_t& zmq_ctx,
               net::io_context& ioc,
               ssl::context& ssl_ctx)
    : host_(host), port_(port), endpoint_(endpoint),
      ssl_ctx_(ssl_ctx), ioc_(ioc),
      ws_(ioc, ssl_ctx_),
      publisher_(zmq_ctx, zmq::socket_type::pub) {

    ssl_ctx_.set_default_verify_paths();
    publisher_.bind("tcp://*:5556");
    std::cout << "ZeroMQ Publisher for OKX started at tcp://*:5556\n";
}

awaitable<void> OkxWss::run() {
    try {
        tcp::resolver resolver(co_await net::this_coro::executor);
        auto endpoints = co_await resolver.async_resolve(host_, port_, use_awaitable);

        co_await net::async_connect(ws_.next_layer().next_layer(), endpoints, use_awaitable);

        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw boost::system::system_error{ec};
        }

        co_await ws_.next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
        co_await ws_.async_handshake(host_ + ":" + port_, endpoint_, use_awaitable);

        std::cout << "Connected to OKX WebSocket (co_await)\n";

        subscribe("books5", "BTC-USDT");

        beast::flat_buffer buffer;

        while (true) {
            buffer.clear();
            co_await ws_.async_read(buffer, use_awaitable);
            std::string msg = beast::buffers_to_string(buffer.data());

            auto j = json::parse(msg, nullptr, false);
            if (j.is_discarded()) continue;

            if (j.contains("data") && j["data"].is_array() && !j["data"].empty()) {
                auto data = j["data"][0];

                if (data.contains("bids") && data.contains("asks") &&
                    !data["bids"].empty() && !data["asks"].empty()) {

                    std::string bid = data["bids"][0][0];
                    std::string bidQty = data["bids"][0][1];
                    std::string ask = data["asks"][0][0];
                    std::string askQty = data["asks"][0][1];

                    json message = {
                        {"exchange", "okx"},
                        {"bid", bid},
                        {"bidQty", bidQty},
                        {"ask", ask},
                        {"askQty", askQty}
                    };

                    zmq::message_t topic("okx", 3);
                    zmq::message_t payload(message.dump());

                    publisher_.send(topic, zmq::send_flags::sndmore);
                    publisher_.send(payload, zmq::send_flags::none);

                    std::cout << "[ZMQ Sent] " << message.dump() << std::endl;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[OKX Coroutine Error] " << e.what() << "\n";
    }
    co_return;
}

void OkxWss::subscribe(const std::string& channel, const std::string& instrument) {
    json sub = {
        {"op", "subscribe"},
        {"args", {
            {
                {"channel", channel},
                {"instId", instrument}
            }
        }}
    };
    ws_.write(net::buffer(sub.dump()));
    std::cout << "Subscribed to " << channel << " | " << instrument << "\n";
}

void OkxWss::close() {
    ws_.close(websocket::close_code::normal);
}

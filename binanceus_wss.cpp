#include "binanceus_wss.hpp"
#include <iostream>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;
using net::awaitable;
using net::use_awaitable;

BinanceUsWss::BinanceUsWss(const std::string& host, const std::string& port,
                           const std::string& endpoint,
                           zmq::context_t& zmq_ctx,
                           net::io_context& ioc,
                           ssl::context& ssl_ctx)
    : host_(host), port_(port), endpoint_(endpoint),
      ssl_ctx_(ssl_ctx), ioc_(ioc),
      ws_(ioc, ssl_ctx_),
      publisher_(zmq_ctx, zmq::socket_type::pub) {

    ssl_ctx_.set_default_verify_paths();
    publisher_.bind("tcp://*:5555");
    std::cout << "ZeroMQ Publisher started at tcp://*:5555\n";
}

awaitable<void> BinanceUsWss::run() {
    try {
        tcp::resolver resolver(co_await net::this_coro::executor);
        auto endpoints = co_await resolver.async_resolve(host_, port_, use_awaitable);

        co_await net::async_connect(ws_.next_layer().next_layer(), endpoints, use_awaitable);
        co_await ws_.next_layer().async_handshake(ssl::stream_base::client, use_awaitable);
        co_await ws_.async_handshake(host_ + ":" + port_, endpoint_, use_awaitable);

        std::cout << "Connected to Binance.US WebSocket (co_await)\n";

        subscribe("btcusdt@bookTicker");

        beast::flat_buffer buffer;

        while (true) {
            buffer.clear();
            co_await ws_.async_read(buffer, use_awaitable);
            std::string msg = beast::buffers_to_string(buffer.data());

            auto j = json::parse(msg, nullptr, false);
            if (j.is_discarded()) continue;

            if (j.contains("b") && j.contains("a")) {
                std::string bid = j["b"];
                std::string bidQty = j["B"];
                std::string ask = j["a"];
                std::string askQty = j["A"];

                json message = {
                    {"exchange", "binanceus"},
                    {"bid", bid},
                    {"bidQty", bidQty},
                    {"ask", ask},
                    {"askQty", askQty}
                };

                zmq::message_t topic("binance", 7);
                zmq::message_t payload(message.dump());

                publisher_.send(topic, zmq::send_flags::sndmore);
                publisher_.send(payload, zmq::send_flags::none);

                std::cout << "[ZMQ Sent] " << message.dump() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Binance Coroutine Error] " << e.what() << "\n";
    }
    co_return;
}

void BinanceUsWss::subscribe(const std::string& channel) {
    json sub = {
        {"method", "SUBSCRIBE"},
        {"params", {channel}},
        {"id", 1}
    };
    ws_.write(net::buffer(sub.dump()));
    std::cout << "Subscribed to " << channel << "\n";
}

void BinanceUsWss::close() {
    ws_.close(websocket::close_code::normal);
}

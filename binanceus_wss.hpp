// binanceus_wss.hpp
#pragma once
#include <iostream>
#include "base_wss_client.hpp"

class BinanceUsWss : public BaseWssClient {
public:
    BinanceUsWss(const std::string& host, const std::string& port, const std::string& endpoint,
                 zmq::context_t& zmq_ctx, net::io_context& ioc, ssl::context& ssl_ctx)
        : BaseWssClient(host, port, endpoint, "tcp://*:5555", zmq_ctx, ioc, ssl_ctx) {}

protected:
    void subscribe_channels() override {
        json sub = {
            {"method", "SUBSCRIBE"},
            {"params", {"btcusdt@bookTicker"}},
            {"id", 1}
        };
        ws_.write(net::buffer(sub.dump()));
        std::cout << "Subscribed to binanceus btcusdt@bookTicker\n";
    }

    void handle_message(const std::string& raw_msg) override {
        auto j = json::parse(raw_msg, nullptr, false);
        if (j.is_discarded()) return;

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

            publish("binance", message);
        }
    }
};

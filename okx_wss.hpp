// okx_wss.hpp
#pragma once
#include <iostream>
#include "base_wss_client.hpp"

class OkxWss : public BaseWssClient {
public:
    OkxWss(const std::string& host, const std::string& port, const std::string& endpoint,
           zmq::context_t& zmq_ctx, net::io_context& ioc, ssl::context& ssl_ctx)
        : BaseWssClient(host, port, endpoint, "tcp://*:5556", zmq_ctx, ioc, ssl_ctx) {}

protected:
    void subscribe_channels() override {
        json sub = {
            {"op", "subscribe"},
            {"args", {{{"channel", "books5"}, {"instId", "BTC-USDT"}}}}
        };
        ws_.write(net::buffer(sub.dump()));
        std::cout << "Subscribed to OKX books5 | BTC-USDT\n";
    }

    void handle_message(const std::string& raw_msg) override {
        auto j = json::parse(raw_msg, nullptr, false);
        if (j.is_discarded()) return;

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

                publish("okx", message);
            }
        }
    }
};

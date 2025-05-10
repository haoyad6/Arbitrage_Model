#include "arbitrage_monitor.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ArbitrageMonitor::ArbitrageMonitor(zmq::context_t& context, const std::vector<std::string>& endpoints)
    : subscriber_(context, zmq::socket_type::sub) {
    for (const auto& ep : endpoints) {
        subscriber_.connect(ep);
    }

    subscriber_.set(zmq::sockopt::subscribe, "binance");
    subscriber_.set(zmq::sockopt::subscribe, "okx");
}

void ArbitrageMonitor::start() {
    while (true) {
        zmq::message_t topic_msg;
        zmq::message_t data_msg;

        if (!subscriber_.recv(topic_msg, zmq::recv_flags::none) ||
            !subscriber_.recv(data_msg, zmq::recv_flags::none)) {
            continue;
        }

        std::string topic(static_cast<char*>(topic_msg.data()), topic_msg.size());
        std::string payload(static_cast<char*>(data_msg.data()), data_msg.size());

        try {
            json j = json::parse(payload);

            PriceInfo info;
            info.bid = std::stod(j["bid"].get<std::string>());
            info.bidQty = std::stod(j["bidQty"].get<std::string>());
            info.ask = std::stod(j["ask"].get<std::string>());
            info.askQty = std::stod(j["askQty"].get<std::string>());

            strategy_.updatePrice(j["exchange"].get<std::string>(), info);
        } catch (...) {
            std::cerr << "Failed to parse: " << payload << std::endl;
        }
    }
}

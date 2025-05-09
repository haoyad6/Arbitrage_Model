#include "arbitrage_monitor.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ArbitrageMonitor::ArbitrageMonitor(zmq::context_t& context, const std::vector<std::string>& endpoints)
    : subscriber_(context, zmq::socket_type::sub) {
    for (const auto& ep : endpoints) {
        subscriber_.connect(ep);
        std::cout << "ArbitrageMonitor subscribed to " << ep << std::endl;
    }

    subscriber_.set(zmq::sockopt::subscribe, "binance");
    subscriber_.set(zmq::sockopt::subscribe, "okx");
}

void ArbitrageMonitor::start() {
    while (true) {
        zmq::message_t topic_msg;
        zmq::message_t data_msg;

        auto result1 = subscriber_.recv(topic_msg, zmq::recv_flags::none);
        auto result2 = subscriber_.recv(data_msg, zmq::recv_flags::none);

        if (!result1 || !result2) {
            std::cerr << "ZMQ 接收失败！" << std::endl;
            continue;
        }

        std::string topic(static_cast<char*>(topic_msg.data()), topic_msg.size());
        std::string payload(static_cast<char*>(data_msg.data()), data_msg.size());

        try {
            json j = json::parse(payload);

            std::cout << "[Monitor] Exchange: " << j["exchange"]
                      << " | 买一: " << j["bid"] << " (量: " << j["bidQty"] << ")"
                      << " | 卖一: " << j["ask"] << " (量: " << j["askQty"] << ")"
                      << std::endl;
        } catch (...) {
            std::cerr << "Failed to parse payload: " << payload << std::endl;
        }
    }
}

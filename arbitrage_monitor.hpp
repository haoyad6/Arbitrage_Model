#pragma once
#include <zmq.hpp>
#include <vector>
#include <string>
#include "arbitrage_strategy.hpp"

class ArbitrageMonitor {
public:
    ArbitrageMonitor(zmq::context_t& context, const std::vector<std::string>& endpoints);
    void start();

private:
    zmq::socket_t subscriber_;
    ArbitrageStrategy strategy_;
};

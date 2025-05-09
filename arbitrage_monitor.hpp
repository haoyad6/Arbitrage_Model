#pragma once

#include <zmq.hpp>
#include <string>
#include <vector>

class ArbitrageMonitor {
public:
    ArbitrageMonitor(zmq::context_t& context, const std::vector<std::string>& endpoints);
    void start();

private:
    zmq::socket_t subscriber_;
};

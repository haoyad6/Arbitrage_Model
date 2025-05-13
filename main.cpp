// main.cpp
#include "binanceus_wss.hpp"
#include "okx_wss.hpp"
#include "arbitrage_monitor.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <zmq.hpp>
#include <iostream>
#include <thread>

int main() {
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12_client);
    zmq::context_t zmq_ctx(1);

    BinanceUsWss binance("stream.binance.us", "9443", "/ws", zmq_ctx, ioc, ssl_ctx);
    OkxWss okx("ws.okx.com", "8443", "/ws/v5/public", zmq_ctx, ioc, ssl_ctx);

    boost::asio::co_spawn(ioc, binance.run(), boost::asio::detached);
    boost::asio::co_spawn(ioc, okx.run(), boost::asio::detached);

    std::thread monitor_thread([&zmq_ctx]() {
        ArbitrageMonitor monitor(zmq_ctx, {"tcp://localhost:5555", "tcp://localhost:5556"});
        monitor.start();
    });

    ioc.run();
    monitor_thread.join();
    return 0;
}


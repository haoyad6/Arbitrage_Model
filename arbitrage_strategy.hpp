// arbitrage_strategy.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

struct PriceInfo {
    double bid, ask;
    double bidQty, askQty;
};

class ArbitrageStrategy {
public:
    ArbitrageStrategy();

    void updatePrice(const std::string& exchange, const PriceInfo& price);
    void evaluate();  // 判断套利机会

private:
    std::unordered_map<std::string, PriceInfo> latest_prices_;
    std::mutex mutex_;

    const double fee_rate_ = 0;  // 假设手续费千分之一

    void checkArbitrage(const std::string& ex1, const std::string& ex2);
};

#include "arbitrage_strategy.hpp"
#include <iostream>
#include <algorithm>

ArbitrageStrategy::ArbitrageStrategy() {}

void ArbitrageStrategy::updatePrice(const std::string& exchange, const PriceInfo& price) {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_prices_[exchange] = price;
    if (latest_prices_.size() == 2) {
        evaluate();
    }
}

void ArbitrageStrategy::evaluate() {
    std::lock_guard<std::mutex> lock(mutex_);
    checkArbitrage("binanceus", "okx");
    checkArbitrage("okx", "binanceus");
}

void ArbitrageStrategy::checkArbitrage(const std::string& buyEx, const std::string& sellEx) {
    const auto& buy = latest_prices_[buyEx];
    const auto& sell = latest_prices_[sellEx];

    double effective_buy = buy.ask * (1 + fee_rate_);
    double effective_sell = sell.bid * (1 - fee_rate_);

    if (effective_sell > effective_buy) {
        double volume = std::min(buy.askQty, sell.bidQty);
        double profit = (effective_sell - effective_buy) * volume;

        std::cout << "[Arbitrage Opportunity] Buy from " << buyEx
                  << " at " << buy.ask << ", Sell to " << sellEx << " at " << sell.bid
                  << " | Volume: " << volume
                  << " | Estimated Profit: " << profit << std::endl;
    }
}

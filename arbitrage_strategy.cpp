#include "arbitrage_strategy.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>

ArbitrageStrategy::ArbitrageStrategy() {}

void ArbitrageStrategy::updatePrice(const std::string& exchange, const PriceInfo& price) {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_prices_[exchange] = price;

    // 只有两个交易所的最新数据都存在时，才执行套利判断
    if (latest_prices_.count("binanceus") && latest_prices_.count("okx")) {
        if (exchange == "binanceus") {
            checkArbitrage("binanceus", "okx"); // Buy binanceus, Sell okx
            checkArbitrage("okx", "binanceus"); // Buy okx, Sell binanceus
        } else if (exchange == "okx") {
            checkArbitrage("okx", "binanceus"); // Buy okx, Sell binanceus
            checkArbitrage("binanceus", "okx"); // Buy binanceus, Sell okx
        }
    }
}

void ArbitrageStrategy::checkArbitrage(const std::string& buyEx, const std::string& sellEx) {
    const auto& buy = latest_prices_[buyEx];
    const auto& sell = latest_prices_[sellEx];

    double effective_buy = buy.ask * (1 + fee_rate_);
    double effective_sell = sell.bid * (1 - fee_rate_);

    // 放宽条件便于调试 + 输出所有潜在机会
    if (effective_sell > effective_buy) {
        double volume = std::min(buy.askQty, sell.bidQty);
        double profit = (effective_sell - effective_buy) * volume;

        std::cout << std::fixed << std::setprecision(8)
                  << "[Arbitrage Opportunity] Buy from " << buyEx
                  << " at " << buy.ask << ", Sell to " << sellEx << " at " << sell.bid
                  << " | Volume: " << volume
                  << " | Estimated Profit: " << profit << std::endl;
    }

    // 可选调试输出
    std::cout << std::fixed << std::setprecision(8)
              << "[DEBUG] Try arbitrage: buy@" << buyEx << " ask=" << buy.ask
              << ", sell@" << sellEx << " bid=" << sell.bid
              << ", diff=" << sell.bid - buy.ask << std::endl;
}

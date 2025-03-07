#include "CurrencyPair.h"
#include <sstream>
#include <iomanip>
#include <unordered_map>

CurrencyPair::CurrencyPair() 
    : pairCode(""), baseCurrency(""), quoteCurrency(""), price(0.0),
      dayChange(0.0), percentChange(0.0), weeklyChange(0.0), monthlyChange(0.0),
      ytdChange(0.0), yoyChange(0.0), group(""), timestamp("") {
}

CurrencyPair::CurrencyPair(const std::string& pairCode, double price)
    : pairCode(pairCode), price(price), dayChange(0.0), percentChange(0.0),
      weeklyChange(0.0), monthlyChange(0.0), ytdChange(0.0), yoyChange(0.0),
      group(""), timestamp("") {
    
    // Parse base and quote currencies from pair code
    size_t slashPos = pairCode.find('/');
    if (slashPos != std::string::npos) {
        baseCurrency = pairCode.substr(0, slashPos);
        quoteCurrency = pairCode.substr(slashPos + 1);
    }
}

CurrencyPair::CurrencyPair(const std::string& baseCurrency, const std::string& quoteCurrency,
                         double price, double percentChange)
    : baseCurrency(baseCurrency), quoteCurrency(quoteCurrency), price(price),
      dayChange(0.0), percentChange(percentChange), weeklyChange(0.0),
      monthlyChange(0.0), ytdChange(0.0), yoyChange(0.0), group(""), timestamp("") {
    
    // Create pair code from base and quote currencies
    pairCode = baseCurrency + "/" + quoteCurrency;
}

CurrencyPair::CurrencyPair(const std::string& pairCode, double price, double dayChange,
                         double percentChange, double weeklyChange, double monthlyChange,
                         double ytdChange, double yoyChange, const std::string& group,
                         const std::string& timestamp)
    : pairCode(pairCode), price(price), dayChange(dayChange), percentChange(percentChange),
      weeklyChange(weeklyChange), monthlyChange(monthlyChange), ytdChange(ytdChange),
      yoyChange(yoyChange), group(group), timestamp(timestamp) {
    
    // Parse base and quote currencies from pair code
    size_t slashPos = pairCode.find('/');
    if (slashPos != std::string::npos) {
        baseCurrency = pairCode.substr(0, slashPos);
        quoteCurrency = pairCode.substr(slashPos + 1);
    }
}

void CurrencyPair::setPairCode(const std::string& code) {
    pairCode = code;
    
    // Update base and quote currencies
    size_t slashPos = pairCode.find('/');
    if (slashPos != std::string::npos) {
        baseCurrency = pairCode.substr(0, slashPos);
        quoteCurrency = pairCode.substr(slashPos + 1);
    }
}

void CurrencyPair::setPrice(double newPrice) {
    // Calculate day change based on new price
    dayChange = newPrice - price;
    
    // Update price
    price = newPrice;
}

double CurrencyPair::getChangeByMetric(const std::string& metric) const {
    if (metric == "Percent Change") {
        return percentChange;
    } else if (metric == "Weekly") {
        return weeklyChange;
    } else if (metric == "Monthly") {
        return monthlyChange;
    } else if (metric == "YTD") {
        return ytdChange;
    } else if (metric == "YoY") {
        return yoyChange;
    } else {
        return 0.0;
    }
}

std::string CurrencyPair::toString() const {
    std::stringstream ss;
    ss << pairCode << ": " << std::fixed << std::setprecision(4) << price;
    
    if (percentChange != 0.0) {
        ss << " (" << (percentChange >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << percentChange << "%)";
    }
    
    return ss.str();
}

CurrencyPair CurrencyPair::getInvertedPair() const {
    // Create inverted pair (e.g., USD/INR -> INR/USD)
    std::string invertedPairCode = quoteCurrency + "/" + baseCurrency;
    
    // Calculate inverted price
    double invertedPrice = 1.0 / price;
    
    // Invert the percentage changes
    // For percentage changes, we need to use a formula to properly invert:
    // invertedChange = (1/(1 + change/100) - 1) * 100
    auto invertPercentage = [](double change) -> double {
        return (1.0 / (1.0 + change / 100.0) - 1.0) * 100.0;
    };
    
    return CurrencyPair(
        invertedPairCode,
        invertedPrice,
        -dayChange / (price * price),  // Inverted day change
        invertPercentage(percentChange),
        invertPercentage(weeklyChange),
        invertPercentage(monthlyChange),
        invertPercentage(ytdChange),
        invertPercentage(yoyChange),
        group,
        timestamp
    );
}
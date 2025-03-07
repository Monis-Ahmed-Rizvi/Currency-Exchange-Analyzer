#pragma once

#include <string>
#include <map>

class CurrencyPair {
private:
    std::string pairCode;       // e.g., "USD/INR"
    std::string baseCurrency;   // e.g., "USD"
    std::string quoteCurrency;  // e.g., "INR"
    double price;               // Current price
    double dayChange;           // Day change amount
    double percentChange;       // Day change percentage
    double weeklyChange;        // Weekly change percentage
    double monthlyChange;       // Monthly change percentage
    double ytdChange;           // Year-to-date change percentage
    double yoyChange;           // Year-over-year change percentage
    std::string group;          // Currency group (e.g., "Major", "Asia")
    std::string timestamp;      // Last update timestamp

public:
    // Constructors
    CurrencyPair();
    CurrencyPair(const std::string& pairCode, double price);
    CurrencyPair(const std::string& baseCurrency, const std::string& quoteCurrency, 
                double price, double percentChange);
    
    // Full constructor
    CurrencyPair(const std::string& pairCode, double price, double dayChange, 
                double percentChange, double weeklyChange, double monthlyChange,
                double ytdChange, double yoyChange, const std::string& group,
                const std::string& timestamp);
    
    // Getters
    std::string getPairCode() const { return pairCode; }
    std::string getBaseCurrency() const { return baseCurrency; }
    std::string getQuoteCurrency() const { return quoteCurrency; }
    double getPrice() const { return price; }
    double getDayChange() const { return dayChange; }
    double getPercentChange() const { return percentChange; }
    double getWeeklyChange() const { return weeklyChange; }
    double getMonthlyChange() const { return monthlyChange; }
    double getYtdChange() const { return ytdChange; }
    double getYoyChange() const { return yoyChange; }
    std::string getGroup() const { return group; }
    std::string getTimestamp() const { return timestamp; }
    
    // Setters
    void setPairCode(const std::string& code);
    void setPrice(double newPrice);
    void setDayChange(double change) { dayChange = change; }
    void setPercentChange(double change) { percentChange = change; }
    void setWeeklyChange(double change) { weeklyChange = change; }
    void setMonthlyChange(double change) { monthlyChange = change; }
    void setYtdChange(double change) { ytdChange = change; }
    void setYoyChange(double change) { yoyChange = change; }
    void setGroup(const std::string& newGroup) { group = newGroup; }
    void setTimestamp(const std::string& newTimestamp) { timestamp = newTimestamp; }
    
    // Helper methods
    double getChangeByMetric(const std::string& metric) const;
    std::string toString() const;
    
    // Invert a currency pair (e.g., USD/INR -> INR/USD)
    CurrencyPair getInvertedPair() const;
};
#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "CurrencyPair.h"

class CurrencyAnalyzer {
private:
    // Storage for all currency pairs
    std::vector<CurrencyPair> currencyPairs;
    
    // Maps to store currency data for quick lookup
    std::unordered_map<std::string, double> baseRates;  // Rates to USD
    
    // Historical data storage (date -> currency pair -> rate)
    std::map<std::string, std::unordered_map<std::string, double>> historicalData;
    
    // Threshold for significant movement detection
    double movementThreshold = 0.5;  // in percentage
    
    // Calculate base rates for all currencies to USD
    void calculateBaseRates();
    
    // Internal helper for opportunity identification
    std::vector<std::pair<std::string, double>> findArbitrageOpportunities();

public:
    CurrencyAnalyzer();
    ~CurrencyAnalyzer();
    
    // Load data from JSON (output from Python script)
    bool loadFromJson(const std::string& filePath);
    
    // Load data from CSV (alternative output from Python script)
    bool loadFromCsv(const std::string& filePath);
    
    // Get current rate for any currency pair (even those not in the original data)
    double getExchangeRate(const std::string& fromCurrency, const std::string& toCurrency);
    
    // Get top performing currencies
    std::vector<CurrencyPair> getTopPerformers(const std::string& metric, int count);
    
    // Get worst performing currencies
    std::vector<CurrencyPair> getWorstPerformers(const std::string& metric, int count);
    
    // Calculate a custom currency pair rate (e.g., INR to JPY)
    double calculateCrossCurrencyRate(const std::string& fromCurrency, 
                                     const std::string& toCurrency);
    
    // Get all available currency pairs
    std::vector<std::string> getAvailableCurrencyPairs() const;
    
    // Get all available currencies
    std::vector<std::string> getAvailableCurrencies() const;
    
    // Analyze for significant movements
    std::vector<std::string> detectSignificantMovements();
    
    // Find trading opportunities
    std::vector<std::string> identifyTradingOpportunities();
    
    // Save analysis results to file
    bool saveAnalysisToFile(const std::string& filePath);
    
    // Get historical data for a specific currency pair
    std::vector<std::pair<std::string, double>> getHistoricalData(
        const std::string& currencyPair, int limit = 30);
};
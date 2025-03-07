#include "CurrencyAnalyzer.h"
#include "DataReader.h"
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <set>

CurrencyAnalyzer::CurrencyAnalyzer() {
    // Default constructor
}

CurrencyAnalyzer::~CurrencyAnalyzer() {
    // Destructor
}

bool CurrencyAnalyzer::loadFromJson(const std::string& filePath) {
    try {
        // Use the DataReader to load currency pairs from JSON
        currencyPairs = DataReader::readFromJson(filePath);
        
        if (currencyPairs.empty()) {
            std::cerr << "No currency data found in JSON file." << std::endl;
            return false;
        }
        
        // Calculate base rates for cross-currency conversions
        calculateBaseRates();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading JSON file: " << e.what() << std::endl;
        return false;
    }
}

bool CurrencyAnalyzer::loadFromCsv(const std::string& filePath) {
    try {
        // Use the DataReader to load currency pairs from CSV
        currencyPairs = DataReader::readFromCsv(filePath);
        
        if (currencyPairs.empty()) {
            std::cerr << "No currency data found in CSV file." << std::endl;
            return false;
        }
        
        // Calculate base rates for cross-currency conversions
        calculateBaseRates();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading CSV file: " << e.what() << std::endl;
        return false;
    }
}

void CurrencyAnalyzer::calculateBaseRates() {
    // Clear existing base rates
    baseRates.clear();
    
    // First, find the direct USD rates
    for (const auto& pair : currencyPairs) {
        std::string baseCurrency = pair.getBaseCurrency();
        std::string quoteCurrency = pair.getQuoteCurrency();
        double rate = pair.getPrice();
        
        if (baseCurrency == "USD") {
            // Direct USD rate (e.g., USD/INR)
            baseRates[quoteCurrency] = rate;
        } else if (quoteCurrency == "USD") {
            // Inverse USD rate (e.g., EUR/USD)
            baseRates[baseCurrency] = 1.0 / rate;
        }
    }
    
    // Set USD base rate
    baseRates["USD"] = 1.0;
    
    // For currencies without direct USD rates, try to derive through other pairs
    bool ratesUpdated = true;
    while (ratesUpdated) {
        ratesUpdated = false;
        
        for (const auto& pair : currencyPairs) {
            std::string baseCurrency = pair.getBaseCurrency();
            std::string quoteCurrency = pair.getQuoteCurrency();
            double rate = pair.getPrice();
            
            // If we know base currency rate but not quote currency rate
            if (baseRates.find(baseCurrency) != baseRates.end() && 
                baseRates.find(quoteCurrency) == baseRates.end()) {
                baseRates[quoteCurrency] = baseRates[baseCurrency] * rate;
                ratesUpdated = true;
            }
            // If we know quote currency rate but not base currency rate
            else if (baseRates.find(quoteCurrency) != baseRates.end() && 
                     baseRates.find(baseCurrency) == baseRates.end()) {
                baseRates[baseCurrency] = baseRates[quoteCurrency] / rate;
                ratesUpdated = true;
            }
        }
    }
}

double CurrencyAnalyzer::getExchangeRate(const std::string& fromCurrency, const std::string& toCurrency) {
    // Direct lookup - check if this exact pair exists
    std::string pairCode = fromCurrency + "/" + toCurrency;
    for (const auto& pair : currencyPairs) {
        if (pair.getPairCode() == pairCode) {
            return pair.getPrice();
        }
    }
    
    // Inverse lookup - check if the inverse pair exists
    std::string inversePairCode = toCurrency + "/" + fromCurrency;
    for (const auto& pair : currencyPairs) {
        if (pair.getPairCode() == inversePairCode) {
            return 1.0 / pair.getPrice();
        }
    }
    
    // If we don't have a direct rate, calculate via base rates (USD)
    return calculateCrossCurrencyRate(fromCurrency, toCurrency);
}

double CurrencyAnalyzer::calculateCrossCurrencyRate(const std::string& fromCurrency, const std::string& toCurrency) {
    // Check if we have base rates for both currencies
    if (baseRates.find(fromCurrency) != baseRates.end() && baseRates.find(toCurrency) != baseRates.end()) {
        // Calculate cross rate via USD
        return baseRates[toCurrency] / baseRates[fromCurrency];
    }
    
    // If we don't have base rates, search for direct or inverse pairs
    std::string directPair = fromCurrency + "/" + toCurrency;
    std::string inversePair = toCurrency + "/" + fromCurrency;
    
    for (const auto& pair : currencyPairs) {
        if (pair.getPairCode() == directPair) {
            return pair.getPrice();
        } else if (pair.getPairCode() == inversePair) {
            return 1.0 / pair.getPrice();
        }
    }
    
    // If we get here, we couldn't find a way to convert between the currencies
    std::cerr << "Warning: Could not find conversion rate between " << fromCurrency << " and " << toCurrency << std::endl;
    return 0.0;
}

std::vector<CurrencyPair> CurrencyAnalyzer::getTopPerformers(const std::string& metric, int count) {
    // Create a copy of currency pairs
    std::vector<CurrencyPair> sortedPairs = currencyPairs;
    
    // Sort by the specified metric in descending order
    std::sort(sortedPairs.begin(), sortedPairs.end(), 
              [&metric](const CurrencyPair& a, const CurrencyPair& b) {
                  return a.getChangeByMetric(metric) > b.getChangeByMetric(metric);
              });
    
    // Return the top N performers
    int resultCount = std::min(count, static_cast<int>(sortedPairs.size()));
    return std::vector<CurrencyPair>(sortedPairs.begin(), sortedPairs.begin() + resultCount);
}

std::vector<CurrencyPair> CurrencyAnalyzer::getWorstPerformers(const std::string& metric, int count) {
    // Create a copy of currency pairs
    std::vector<CurrencyPair> sortedPairs = currencyPairs;
    
    // Sort by the specified metric in ascending order
    std::sort(sortedPairs.begin(), sortedPairs.end(), 
              [&metric](const CurrencyPair& a, const CurrencyPair& b) {
                  return a.getChangeByMetric(metric) < b.getChangeByMetric(metric);
              });
    
    // Return the worst N performers
    int resultCount = std::min(count, static_cast<int>(sortedPairs.size()));
    return std::vector<CurrencyPair>(sortedPairs.begin(), sortedPairs.begin() + resultCount);
}

std::vector<std::string> CurrencyAnalyzer::getAvailableCurrencyPairs() const {
    std::vector<std::string> pairs;
    for (const auto& pair : currencyPairs) {
        pairs.push_back(pair.getPairCode());
    }
    return pairs;
}

std::vector<std::string> CurrencyAnalyzer::getAvailableCurrencies() const {
    // Use a set to ensure unique currency codes
    std::set<std::string> currencies;
    
    for (const auto& pair : currencyPairs) {
        currencies.insert(pair.getBaseCurrency());
        currencies.insert(pair.getQuoteCurrency());
    }
    
    // Convert set to vector
    return std::vector<std::string>(currencies.begin(), currencies.end());
}

std::vector<std::string> CurrencyAnalyzer::detectSignificantMovements() {
    std::vector<std::string> significantMovements;
    
    for (const auto& pair : currencyPairs) {
        double change = std::abs(pair.getPercentChange());
        
        if (change > movementThreshold) {
            std::stringstream ss;
            ss << pair.getPairCode() << ": " 
               << (pair.getPercentChange() > 0 ? "UP " : "DOWN ")
               << std::fixed << std::setprecision(2) << std::abs(pair.getPercentChange()) << "% "
               << "to " << std::fixed << std::setprecision(4) << pair.getPrice();
            
            significantMovements.push_back(ss.str());
        }
    }
    
    return significantMovements;
}

std::vector<std::string> CurrencyAnalyzer::identifyTradingOpportunities() {
    std::vector<std::string> opportunities;
    
    // Check for high volatility pairs (both daily and weekly)
    for (const auto& pair : currencyPairs) {
        // Look for pairs with significant daily movement
        if (std::abs(pair.getPercentChange()) > 1.0) {
            std::stringstream ss;
            ss << "High Volatility: " << pair.getPairCode() 
               << " moved " << std::fixed << std::setprecision(2) << std::abs(pair.getPercentChange()) << "% today";
            opportunities.push_back(ss.str());
        }
        
        // Look for reversal patterns (daily opposite to weekly)
        if (pair.getPercentChange() * pair.getWeeklyChange() < 0 && 
            std::abs(pair.getPercentChange()) > 0.5) {
            std::stringstream ss;
            ss << "Potential Reversal: " << pair.getPairCode() 
               << " is " << (pair.getPercentChange() > 0 ? "up" : "down") << " " 
               << std::fixed << std::setprecision(2) << std::abs(pair.getPercentChange()) << "% today, but "
               << (pair.getWeeklyChange() > 0 ? "up" : "down") << " " 
               << std::fixed << std::setprecision(2) << std::abs(pair.getWeeklyChange()) << "% this week";
            opportunities.push_back(ss.str());
        }
    }
    
    // Check for arbitrage opportunities
    auto arbitrageOpps = findArbitrageOpportunities();
    for (const auto& opp : arbitrageOpps) {
        opportunities.push_back("Arbitrage Opportunity: " + opp.first + " (" + 
                                std::to_string(opp.second) + "% potential)");
    }
    
    return opportunities;
}

std::vector<std::pair<std::string, double>> CurrencyAnalyzer::findArbitrageOpportunities() {
    std::vector<std::pair<std::string, double>> opportunities;
    
    // A simple triangular arbitrage check
    // For each currency A, B, C, check if A→B→C→A gives a profit
    std::vector<std::string> currencies = getAvailableCurrencies();
    
    for (const auto& currA : currencies) {
        for (const auto& currB : currencies) {
            if (currA == currB) continue;
            
            for (const auto& currC : currencies) {
                if (currA == currC || currB == currC) continue;
                
                double rateAB = calculateCrossCurrencyRate(currA, currB);
                double rateBC = calculateCrossCurrencyRate(currB, currC);
                double rateCA = calculateCrossCurrencyRate(currC, currA);
                
                if (rateAB <= 0 || rateBC <= 0 || rateCA <= 0) continue;
                
                // Calculate arbitrage profit percentage
                double profit = (rateAB * rateBC * rateCA - 1.0) * 100;
                
                // If profit exceeds a threshold (e.g., 1%), consider it an opportunity
                if (profit > 1.0) {
                    std::stringstream ss;
                    ss << currA << "→" << currB << "→" << currC << "→" << currA;
                    opportunities.push_back(std::make_pair(ss.str(), profit));
                }
            }
        }
    }
    
    return opportunities;
}

bool CurrencyAnalyzer::saveAnalysisToFile(const std::string& filePath) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }
        
        // Write header
        file << "Currency Analysis Report\n";
        file << "=======================\n\n";
        
        // Top performers by daily change
        file << "Top 5 Daily Performers:\n";
        auto topDaily = getTopPerformers("Percent Change", 5);
        for (const auto& pair : topDaily) {
            file << "- " << pair.getPairCode() << ": " 
                 << std::fixed << std::setprecision(2) << pair.getPercentChange() << "%\n";
        }
        file << "\n";
        
        // Worst performers by daily change
        file << "Worst 5 Daily Performers:\n";
        auto worstDaily = getWorstPerformers("Percent Change", 5);
        for (const auto& pair : worstDaily) {
            file << "- " << pair.getPairCode() << ": " 
                 << std::fixed << std::setprecision(2) << pair.getPercentChange() << "%\n";
        }
        file << "\n";
        
        // Significant movements
        file << "Significant Movements:\n";
        auto movements = detectSignificantMovements();
        if (movements.empty()) {
            file << "No significant movements detected.\n";
        } else {
            for (const auto& movement : movements) {
                file << "- " << movement << "\n";
            }
        }
        file << "\n";
        
        // Trading opportunities
        file << "Trading Opportunities:\n";
        auto opportunities = identifyTradingOpportunities();
        if (opportunities.empty()) {
            file << "No trading opportunities identified.\n";
        } else {
            for (const auto& opportunity : opportunities) {
                file << "- " << opportunity << "\n";
            }
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving analysis to file: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::pair<std::string, double>> CurrencyAnalyzer::getHistoricalData(
    const std::string& currencyPair, int limit) {
    
    // In a real implementation, this would retrieve data from a database
    // For now, we'll just return the current price with some fake history
    
    std::vector<std::pair<std::string, double>> history;
    
    // Find the current price for this pair
    double currentPrice = 0.0;
    std::string timestamp;
    
    for (const auto& pair : currencyPairs) {
        if (pair.getPairCode() == currencyPair) {
            currentPrice = pair.getPrice();
            timestamp = pair.getTimestamp();
            break;
        }
    }
    
    if (currentPrice == 0.0) {
        // Pair not found
        return history;
    }
    
    // Create fake historical data based on current price
    // In a real implementation, this would come from historical database
    for (int i = 0; i < limit; ++i) {
        // Add some random variation to create fake history
        double randomFactor = 1.0 + ((rand() % 100 - 50) / 5000.0);
        double historicalPrice = currentPrice * randomFactor;
        
        // Create a fake timestamp (just for demonstration)
        std::stringstream ss;
        ss << "2025-03-" << std::setw(2) << std::setfill('0') << (7 - i) << " " 
           << std::setw(2) << std::setfill('0') << (rand() % 24) << ":" 
           << std::setw(2) << std::setfill('0') << (rand() % 60);
        
        history.push_back(std::make_pair(ss.str(), historicalPrice));
    }
    
    return history;
}
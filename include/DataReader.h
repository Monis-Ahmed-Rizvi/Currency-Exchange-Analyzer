#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "CurrencyPair.h"

// This class handles reading currency data from various file formats
class DataReader {
public:
    // Read currency data from a JSON file
    static std::vector<CurrencyPair> readFromJson(const std::string& filePath);
    
    // Read currency data from a CSV file
    static std::vector<CurrencyPair> readFromCsv(const std::string& filePath);
    
    // Parse a single JSON object into a CurrencyPair
    static CurrencyPair parseCurrencyPairFromJson(const std::string& jsonString);
    
    // Parse a single CSV line into a CurrencyPair
    static CurrencyPair parseCurrencyPairFromCsv(const std::string& csvLine, 
                                                const std::vector<std::string>& headers);
    
    // Extract individual currencies from pairs
    static std::vector<std::string> extractCurrencies(const std::vector<CurrencyPair>& pairs);
    
    // Check if file exists
    static bool fileExists(const std::string& filePath);
};
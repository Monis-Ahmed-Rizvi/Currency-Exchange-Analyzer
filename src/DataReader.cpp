#include "DataReader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

// Simple JSON parsing helpers
std::string getJsonStringValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    
    size_t valueStart = json.find(":", keyPos) + 1;
    while (valueStart < json.length() && std::isspace(json[valueStart])) valueStart++;
    
    if (valueStart >= json.length()) return "";
    
    if (json[valueStart] == '\"') {
        // String value
        valueStart++; // Skip opening quote
        size_t valueEnd = json.find("\"", valueStart);
        return json.substr(valueStart, valueEnd - valueStart);
    } else {
        // Numeric or other value
        size_t valueEnd = json.find_first_of(",}", valueStart);
        return json.substr(valueStart, valueEnd - valueStart);
    }
}

double getJsonDoubleValue(const std::string& json, const std::string& key, double defaultValue = 0.0) {
    std::string strValue = getJsonStringValue(json, key);
    if (strValue.empty()) return defaultValue;
    
    try {
        return std::stod(strValue);
    } catch (...) {
        return defaultValue;
    }
}

std::vector<CurrencyPair> DataReader::readFromJson(const std::string& filePath) {
    std::vector<CurrencyPair> pairs;
    
    // Check if file exists
    if (!fileExists(filePath)) {
        std::cerr << "File does not exist: " << filePath << std::endl;
        return pairs;
    }
    
    // Read the entire file into a string
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    // Simple JSON array parsing
    size_t arrayStart = json.find('[');
    size_t arrayEnd = json.rfind(']');
    
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        std::cerr << "Invalid JSON format: array not found" << std::endl;
        return pairs;
    }
    
    // Extract the array content
    std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // Find each object in the array
    size_t pos = 0;
    while (pos < arrayContent.length()) {
        // Find the start of the object
        size_t objectStart = arrayContent.find('{', pos);
        if (objectStart == std::string::npos) break;
        
        // Find the end of the object
        size_t objectEnd = arrayContent.find('}', objectStart);
        if (objectEnd == std::string::npos) break;
        
        // Extract the object
        std::string objectJson = arrayContent.substr(objectStart, objectEnd - objectStart + 1);
        
        // Parse the object into a CurrencyPair
        CurrencyPair pair = parseCurrencyPairFromJson(objectJson);
        pairs.push_back(pair);
        
        // Move past this object
        pos = objectEnd + 1;
    }
    
    return pairs;
}

CurrencyPair DataReader::parseCurrencyPairFromJson(const std::string& jsonString) {
    // Extract fields from JSON
    std::string pairCode = getJsonStringValue(jsonString, "Currency Pair");
    double price = getJsonDoubleValue(jsonString, "Price");
    double dayChange = getJsonDoubleValue(jsonString, "Day Change");
    double percentChange = getJsonDoubleValue(jsonString, "Percent Change");
    double weeklyChange = getJsonDoubleValue(jsonString, "Weekly");
    double monthlyChange = getJsonDoubleValue(jsonString, "Monthly");
    double ytdChange = getJsonDoubleValue(jsonString, "YTD");
    double yoyChange = getJsonDoubleValue(jsonString, "YoY");
    std::string group = getJsonStringValue(jsonString, "Group");
    std::string timestamp = getJsonStringValue(jsonString, "Timestamp");
    
    return CurrencyPair(
        pairCode, price, dayChange, percentChange, 
        weeklyChange, monthlyChange, ytdChange, yoyChange,
        group, timestamp
    );
}

std::vector<CurrencyPair> DataReader::readFromCsv(const std::string& filePath) {
    std::vector<CurrencyPair> pairs;
    
    // Check if file exists
    if (!fileExists(filePath)) {
        std::cerr << "File does not exist: " << filePath << std::endl;
        return pairs;
    }
    
    // Open the file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << filePath << std::endl;
        return pairs;
    }
    
    // Read header line
    std::string headerLine;
    if (!std::getline(file, headerLine)) {
        std::cerr << "Failed to read CSV header" << std::endl;
        return pairs;
    }
    
    // Parse header to get column positions
    std::vector<std::string> headers;
    std::stringstream headerStream(headerLine);
    std::string header;
    
    // Parse comma-separated headers
    while (std::getline(headerStream, header, ',')) {
        // Remove quotes if present
        if (!header.empty() && header.front() == '"' && header.back() == '"') {
            header = header.substr(1, header.length() - 2);
        }
        // Trim leading/trailing whitespace
        header.erase(0, header.find_first_not_of(" \t"));
        header.erase(header.find_last_not_of(" \t") + 1);
        
        headers.push_back(header);
    }
    
    // Read data lines
    std::string line;
    while (std::getline(file, line)) {
        CurrencyPair pair = parseCurrencyPairFromCsv(line, headers);
        pairs.push_back(pair);
    }
    
    return pairs;
}

CurrencyPair DataReader::parseCurrencyPairFromCsv(const std::string& csvLine, 
                                               const std::vector<std::string>& headers) {
    std::vector<std::string> values;
    std::stringstream lineStream(csvLine);
    std::string cell;
    
    // Parse comma-separated values
    while (std::getline(lineStream, cell, ',')) {
        // Remove quotes if present
        if (!cell.empty() && cell.front() == '"' && cell.back() == '"') {
            cell = cell.substr(1, cell.length() - 2);
        }
        // Trim leading/trailing whitespace
        cell.erase(0, cell.find_first_not_of(" \t"));
        cell.erase(cell.find_last_not_of(" \t") + 1);
        
        values.push_back(cell);
    }
    
    // Ensure values and headers match in size
    if (values.size() != headers.size()) {
        std::cerr << "Warning: CSV line has " << values.size() << " values but expected " 
                  << headers.size() << std::endl;
        
        // Pad values if needed
        while (values.size() < headers.size()) {
            values.push_back("");
        }
    }
    
    // Create a map of field names to values
    std::unordered_map<std::string, std::string> fieldMap;
    for (size_t i = 0; i < headers.size() && i < values.size(); ++i) {
        fieldMap[headers[i]] = values[i];
    }
    
    // Extract currency pair data
    std::string pairCode = fieldMap.count("Currency Pair") ? fieldMap["Currency Pair"] : "";
    
    // Convert numeric fields with error handling
    auto parseDouble = [&fieldMap](const std::string& key, double defaultValue) -> double {
        if (!fieldMap.count(key) || fieldMap[key].empty()) return defaultValue;
        try {
            return std::stod(fieldMap[key]);
        } catch (...) {
            return defaultValue;
        }
    };
    
    double price = parseDouble("Price", 0.0);
    double dayChange = parseDouble("Day Change", 0.0);
    double percentChange = parseDouble("Percent Change", 0.0);
    double weeklyChange = parseDouble("Weekly", 0.0);
    double monthlyChange = parseDouble("Monthly", 0.0);
    double ytdChange = parseDouble("YTD", 0.0);
    double yoyChange = parseDouble("YoY", 0.0);
    
    std::string group = fieldMap.count("Group") ? fieldMap["Group"] : "";
    std::string timestamp = fieldMap.count("Timestamp") ? fieldMap["Timestamp"] : "";
    
    return CurrencyPair(
        pairCode, price, dayChange, percentChange, 
        weeklyChange, monthlyChange, ytdChange, yoyChange,
        group, timestamp
    );
}

std::vector<std::string> DataReader::extractCurrencies(const std::vector<CurrencyPair>& pairs) {
    std::set<std::string> currencies;
    
    for (const auto& pair : pairs) {
        currencies.insert(pair.getBaseCurrency());
        currencies.insert(pair.getQuoteCurrency());
    }
    
    return std::vector<std::string>(currencies.begin(), currencies.end());
}

bool DataReader::fileExists(const std::string& filePath) {
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "CurrencyAnalyzer.h"
#include "CurrencyPair.h"
#include "DataReader.h"

// Forward declarations
void displayMenu();
void displayTopPerformers(CurrencyAnalyzer& analyzer);
void displayWorstPerformers(CurrencyAnalyzer& analyzer);
void convertCurrency(CurrencyAnalyzer& analyzer);
void displaySignificantMovements(CurrencyAnalyzer& analyzer);
void displayTradingOpportunities(CurrencyAnalyzer& analyzer);
void displayHistoricalData(CurrencyAnalyzer& analyzer);
void displayAllRates(CurrencyAnalyzer& analyzer);

int main(int argc, char* argv[]) {
    std::cout << "=======================================" << std::endl;
    std::cout << "Currency Analysis System (C++ Edition)" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Initialize currency analyzer
    CurrencyAnalyzer analyzer;
    
    // Check for command line arguments for file path
    std::string dataFilePath = "currency_data.json";
    if (argc > 1) {
        dataFilePath = argv[1];
    }
    
    std::cout << "Loading data from: " << dataFilePath << std::endl;
    
    // Determine file type and load data
    bool loadSuccess = false;
    if (dataFilePath.substr(dataFilePath.find_last_of(".") + 1) == "json") {
        loadSuccess = analyzer.loadFromJson(dataFilePath);
    } else if (dataFilePath.substr(dataFilePath.find_last_of(".") + 1) == "csv") {
        loadSuccess = analyzer.loadFromCsv(dataFilePath);
    } else {
        std::cerr << "Unsupported file format. Please use .json or .csv files." << std::endl;
        return 1;
    }
    
    if (!loadSuccess) {
        std::cerr << "Failed to load currency data. Exiting..." << std::endl;
        return 1;
    }
    
    std::cout << "Data loaded successfully." << std::endl;
    
    int choice = 0;
    do {
        displayMenu();
        std::cout << "Enter your choice (0-8): ";
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                displayTopPerformers(analyzer);
                break;
            case 2:
                displayWorstPerformers(analyzer);
                break;
            case 3:
                convertCurrency(analyzer);
                break;
            case 4:
                displaySignificantMovements(analyzer);
                break;
            case 5:
                displayTradingOpportunities(analyzer);
                break;
            case 6:
                displayHistoricalData(analyzer);
                break;
            case 7:
                displayAllRates(analyzer);
                break;
            case 8: {
                // Reload data (useful for real-time updates)
                std::cout << "Reloading data from: " << dataFilePath << std::endl;
                if (dataFilePath.substr(dataFilePath.find_last_of(".") + 1) == "json") {
                    loadSuccess = analyzer.loadFromJson(dataFilePath);
                } else {
                    loadSuccess = analyzer.loadFromCsv(dataFilePath);
                }
                if (loadSuccess) {
                    std::cout << "Data reloaded successfully." << std::endl;
                } else {
                    std::cerr << "Failed to reload data." << std::endl;
                }
                break;
            }
            case 0:
                std::cout << "Exiting program. Goodbye!" << std::endl;
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
        
        if (choice != 0) {
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}

void displayMenu() {
    std::cout << "\n======= MENU =======" << std::endl;
    std::cout << "1. Display Top Performing Currencies" << std::endl;
    std::cout << "2. Display Worst Performing Currencies" << std::endl;
    std::cout << "3. Convert Between Currencies" << std::endl;
    std::cout << "4. Show Significant Movements" << std::endl;
    std::cout << "5. Identify Trading Opportunities" << std::endl;
    std::cout << "6. View Historical Data" << std::endl;
    std::cout << "7. Display All Exchange Rates" << std::endl;
    std::cout << "8. Reload Data" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "===================" << std::endl;
}

void displayTopPerformers(CurrencyAnalyzer& analyzer) {
    std::string metrics[] = {"Percent Change", "Weekly", "Monthly", "YTD", "YoY"};
    int metricChoice = 0;
    
    std::cout << "\n--- Select Performance Metric ---" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << i+1 << ". " << metrics[i] << std::endl;
    }
    
    std::cout << "Enter your choice (1-5): ";
    std::cin >> metricChoice;
    
    if (metricChoice < 1 || metricChoice > 5) {
        std::cout << "Invalid choice. Using Daily Change." << std::endl;
        metricChoice = 1;
    }
    
    std::string selectedMetric = metrics[metricChoice-1];
    int count = 5;
    std::cout << "How many top performers to display? ";
    std::cin >> count;
    
    std::vector<CurrencyPair> topPerformers = analyzer.getTopPerformers(selectedMetric, count);
    
    std::cout << "\n--- Top " << count << " Performing Currencies (" << selectedMetric << ") ---" << std::endl;
    std::cout << std::left << std::setw(10) << "Pair" 
              << std::setw(15) << "Price" 
              << std::setw(15) << selectedMetric 
              << std::setw(15) << "Group" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& pair : topPerformers) {
        std::cout << std::left << std::setw(10) << pair.getPairCode() 
                  << std::setw(15) << std::fixed << std::setprecision(4) << pair.getPrice() 
                  << std::setw(15) << std::fixed << std::setprecision(2) << pair.getChangeByMetric(selectedMetric) << "%" 
                  << std::setw(15) << pair.getGroup() << std::endl;
    }
}

void displayWorstPerformers(CurrencyAnalyzer& analyzer) {
    std::string metrics[] = {"Percent Change", "Weekly", "Monthly", "YTD", "YoY"};
    int metricChoice = 0;
    
    std::cout << "\n--- Select Performance Metric ---" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << i+1 << ". " << metrics[i] << std::endl;
    }
    
    std::cout << "Enter your choice (1-5): ";
    std::cin >> metricChoice;
    
    if (metricChoice < 1 || metricChoice > 5) {
        std::cout << "Invalid choice. Using Daily Change." << std::endl;
        metricChoice = 1;
    }
    
    std::string selectedMetric = metrics[metricChoice-1];
    int count = 5;
    std::cout << "How many worst performers to display? ";
    std::cin >> count;
    
    std::vector<CurrencyPair> worstPerformers = analyzer.getWorstPerformers(selectedMetric, count);
    
    std::cout << "\n--- Worst " << count << " Performing Currencies (" << selectedMetric << ") ---" << std::endl;
    std::cout << std::left << std::setw(10) << "Pair" 
              << std::setw(15) << "Price" 
              << std::setw(15) << selectedMetric 
              << std::setw(15) << "Group" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& pair : worstPerformers) {
        std::cout << std::left << std::setw(10) << pair.getPairCode() 
                  << std::setw(15) << std::fixed << std::setprecision(4) << pair.getPrice() 
                  << std::setw(15) << std::fixed << std::setprecision(2) << pair.getChangeByMetric(selectedMetric) << "%" 
                  << std::setw(15) << pair.getGroup() << std::endl;
    }
}

void convertCurrency(CurrencyAnalyzer& analyzer) {
    // Get all available currencies
    std::vector<std::string> currencies = analyzer.getAvailableCurrencies();
    
    // Display available currencies
    std::cout << "\n--- Available Currencies ---" << std::endl;
    int count = 0;
    for (const auto& currency : currencies) {
        std::cout << std::setw(6) << currency;
        if (++count % 10 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // Get from and to currencies
    std::string fromCurrency, toCurrency;
    double amount;
    
    std::cout << "Enter source currency (e.g., USD): ";
    std::cin >> fromCurrency;
    
    std::cout << "Enter target currency (e.g., INR): ";
    std::cin >> toCurrency;
    
    std::cout << "Enter amount to convert: ";
    std::cin >> amount;
    
    // Convert to uppercase
    std::transform(fromCurrency.begin(), fromCurrency.end(), fromCurrency.begin(), ::toupper);
    std::transform(toCurrency.begin(), toCurrency.end(), toCurrency.begin(), ::toupper);
    
    // Check if currencies exist
    if (std::find(currencies.begin(), currencies.end(), fromCurrency) == currencies.end()) {
        std::cout << "Error: Source currency '" << fromCurrency << "' not found." << std::endl;
        return;
    }
    
    if (std::find(currencies.begin(), currencies.end(), toCurrency) == currencies.end()) {
        std::cout << "Error: Target currency '" << toCurrency << "' not found." << std::endl;
        return;
    }
    
    // Get exchange rate and calculate conversion
    double rate = analyzer.calculateCrossCurrencyRate(fromCurrency, toCurrency);
    double result = amount * rate;
    
    std::cout << "\n--- Currency Conversion Result ---" << std::endl;
    std::cout << amount << " " << fromCurrency << " = " << std::fixed << std::setprecision(4) 
              << result << " " << toCurrency << std::endl;
    std::cout << "Exchange Rate: 1 " << fromCurrency << " = " << std::fixed << std::setprecision(6) 
              << rate << " " << toCurrency << std::endl;
}

void displaySignificantMovements(CurrencyAnalyzer& analyzer) {
    std::cout << "\n--- Significant Currency Movements ---" << std::endl;
    
    std::vector<std::string> movements = analyzer.detectSignificantMovements();
    
    if (movements.empty()) {
        std::cout << "No significant movements detected." << std::endl;
        return;
    }
    
    for (const auto& movement : movements) {
        std::cout << movement << std::endl;
    }
}

void displayTradingOpportunities(CurrencyAnalyzer& analyzer) {
    std::cout << "\n--- Trading Opportunities ---" << std::endl;
    
    std::vector<std::string> opportunities = analyzer.identifyTradingOpportunities();
    
    if (opportunities.empty()) {
        std::cout << "No trading opportunities identified." << std::endl;
        return;
    }
    
    for (const auto& opportunity : opportunities) {
        std::cout << opportunity << std::endl;
    }
}

void displayHistoricalData(CurrencyAnalyzer& analyzer) {
    // Get available currency pairs
    std::vector<std::string> pairs = analyzer.getAvailableCurrencyPairs();
    
    // Display available pairs
    std::cout << "\n--- Available Currency Pairs ---" << std::endl;
    int count = 0;
    for (const auto& pair : pairs) {
        std::cout << std::setw(10) << pair;
        if (++count % 6 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // Get currency pair
    std::string currencyPair;
    std::cout << "Enter currency pair to view (e.g., USD/INR): ";
    std::cin >> currencyPair;
    
    // Convert to uppercase
    std::transform(currencyPair.begin(), currencyPair.end(), currencyPair.begin(), ::toupper);
    
    // Get historical data
    std::vector<std::pair<std::string, double>> history = analyzer.getHistoricalData(currencyPair);
    
    if (history.empty()) {
        std::cout << "No historical data available for " << currencyPair << std::endl;
        return;
    }
    
    std::cout << "\n--- Historical Data for " << currencyPair << " ---" << std::endl;
    std::cout << std::left << std::setw(25) << "Timestamp" 
              << std::setw(15) << "Price" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    for (const auto& dataPoint : history) {
        std::cout << std::left << std::setw(25) << dataPoint.first 
                  << std::fixed << std::setprecision(6) << dataPoint.second << std::endl;
    }
    
    // TODO: Add a simple ASCII chart if desired
}

void displayAllRates(CurrencyAnalyzer& analyzer) {
    std::vector<std::string> pairs = analyzer.getAvailableCurrencyPairs();
    
    std::cout << "\n--- All Exchange Rates ---" << std::endl;
    std::cout << std::left << std::setw(10) << "Pair" 
              << std::setw(15) << "Price" 
              << std::setw(15) << "% Change"
              << std::setw(15) << "Group" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    // Get top performers - using it to get all data but will display all
    std::vector<CurrencyPair> allPairs = analyzer.getTopPerformers("Percent Change", pairs.size());
    
    for (const auto& pair : allPairs) {
        // Add color coding for positive/negative changes (works in some terminals)
        std::string color = pair.getPercentChange() >= 0 ? "\033[32m" : "\033[31m"; // Green or Red
        std::string resetColor = "\033[0m";
        
        std::cout << std::left << std::setw(10) << pair.getPairCode() 
                  << std::setw(15) << std::fixed << std::setprecision(4) << pair.getPrice() 
                  << color << std::setw(15) << std::fixed << std::setprecision(2) << pair.getPercentChange() << "%" << resetColor
                  << std::setw(15) << pair.getGroup() << std::endl;
    }
}
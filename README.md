# Currency Exchange Analysis System

![Language](https://img.shields.io/badge/Languages-C++%20%7C%20Python-blue)
![License](https://img.shields.io/badge/License-MIT-green)

A high-performance, cross-language system for real-time currency exchange rate analysis, combining Python web scraping capabilities with C++ data processing and analytics.

![System Architecture](docs/screenshots/architecture.png)

## 🚀 Features

- **Real-time Data Collection**: Scrapes live currency exchange rates from Trading Economics
- **Cross-Currency Calculations**: Calculate exchange rates between any currency pairs
- **Performance Analytics**: Identifies top and worst-performing currencies
- **Movement Detection**: Monitors significant price movements in real-time
- **Trading Opportunity Finder**: Identifies potential arbitrage and trading opportunities
- **C++ & Python Integration**: Leverages Python for web scraping and C++ for performance-critical analysis
- **Historical Data Tracking**: Stores and analyzes historical price movements
- **Interactive CLI**: User-friendly command-line interface for data exploration

## 🛠️ System Architecture

This project demonstrates advanced cross-language integration:

1. **Python Module (Data Collection)**:
   - Web scraping with BeautifulSoup and Requests
   - Data storage in SQLite database
   - JSON/CSV export for cross-language integration

2. **C++ Analytics Engine**:
   - High-performance data processing
   - Cross-currency rate calculations
   - Algorithmic trading opportunity detection
   - CLI user interface

## 📊 Example Analysis

The system identifies various trading insights:

- **Significant Movements**: Detects currencies with unusual price action
- **Arbitrage Opportunities**: Finds potential triangular arbitrage situations
- **Performance Rankings**: Tracks best and worst performers across timeframes
- **Custom Pair Calculations**: Computes rates between any currency pairs

## 📋 Prerequisites

- C++ Compiler (C++11 or newer)
- Python 3.6+
- Python packages:
  - requests
  - beautifulsoup4
  - pandas
  - numpy
  - matplotlib (optional)

## 🔧 Installation

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/Currency-Exchange-Analyzer.git
   cd Currency-Exchange-Analyzer
   ```

2. Install Python dependencies:
   ```
   pip install -r requirements.txt
   ```

3. Compile the C++ code:
   ```
   # On Windows
   compile.bat
   
   # On Linux/macOS
   mkdir -p build
   g++ -std=c++11 -I./include src/cpp/*.cpp -o build/currency_analyzer
   ```

## 🚀 Usage

### Quick Start

The simplest way to run the system:

```bash
python src/python/python_cpp_integration.py --mode oneshot
```

This will:
1. Fetch current exchange rate data
2. Save it to a JSON file
3. Launch the C++ analyzer with the data

### Continuous Mode

For continuous monitoring:

```bash
python src/python/python_cpp_integration.py --mode continuous --refresh 60
```

### Using Existing Data

To analyze previously collected data:

```bash
python src/python/python_cpp_integration.py --mode analyze
```

## 📈 Future Development

- [ ] GUI Dashboard with real-time charts and visualizations
- [ ] Advanced arbitrage detection with profit calculation
- [ ] Machine learning models for rate prediction
- [ ] Email/SMS alerts for significant movements
- [ ] Portfolio risk analysis tools
- [ ] REST API for external integrations

## 🤝 Contributing

Contributions are welcome! Feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgements

- Trading Economics for market data
- The open-source C++ and Python communities

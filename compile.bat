@echo off
echo Compiling Currency Analyzer...

echo Checking g++ version:
g++ --version

if not exist build mkdir build
echo Build directory created or already exists.

echo Compiling CurrencyPair.cpp...
g++ -std=c++11 -I./include -c ./src/CurrencyPair.cpp -o ./build/CurrencyPair.o
if %ERRORLEVEL% NEQ 0 goto error

echo Compiling DataReader.cpp...
g++ -std=c++11 -I./include -c ./src/DataReader.cpp -o ./build/DataReader.o
if %ERRORLEVEL% NEQ 0 goto error

echo Compiling CurrencyAnalyzer.cpp...
g++ -std=c++11 -I./include -c ./src/CurrencyAnalyzer.cpp -o ./build/CurrencyAnalyzer.o
if %ERRORLEVEL% NEQ 0 goto error

echo Compiling main.cpp...
g++ -std=c++11 -I./include -c ./src/main.cpp -o ./build/main.o
if %ERRORLEVEL% NEQ 0 goto error

echo Linking executable...
g++ -o currency_analyzer.exe ./build/CurrencyPair.o ./build/DataReader.o ./build/CurrencyAnalyzer.o ./build/main.o
if %ERRORLEVEL% NEQ 0 goto error

echo Compilation successful!
goto end

:error
echo Compilation failed with error code %ERRORLEVEL%

:end
pause
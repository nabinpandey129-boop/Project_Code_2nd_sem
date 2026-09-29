@echo off
g++ -std=c++17 -O2 -o hms.exe main.cpp -lws2_32
if errorlevel 1 (echo Build failed) else (echo Built hms.exe - double-click it to run)
pause

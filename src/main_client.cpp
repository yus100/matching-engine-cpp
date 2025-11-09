#include "Client.h"
#include "Common.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <vector>

using namespace MatchingEngine;

void printUsage() {
    std::cout << "\nAvailable Commands:" << std::endl;
    std::cout << "  buy <symbol> <quantity> <price>       - Submit a buy limit order" << std::endl;
    std::cout << "  sell <symbol> <quantity> <price>      - Submit a sell limit order" << std::endl;
    std::cout << "  market-buy <symbol> <quantity>        - Submit a market buy order" << std::endl;
    std::cout << "  market-sell <symbol> <quantity>       - Submit a market sell order" << std::endl;
    std::cout << "  cancel <order_id>                     - Cancel an order" << std::endl;
    std::cout << "  modify <order_id> <price> <quantity>  - Modify an order" << std::endl;
    std::cout << "  help                                  - Show this help message" << std::endl;
    std::cout << "  quit                                  - Disconnect and exit" << std::endl;
    std::cout << std::endl;
}

void printWelcome() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Matching Engine Client" << std::endl;
    std::cout << "========================================" << std::endl;
}

std::vector<std::string> splitString(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void runInteractiveMode(Client& client) {
    printUsage();
    
    std::string line;
    std::cout << "> ";
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << "> ";
            continue;
        }
        
        auto tokens = splitString(line);
        if (tokens.empty()) {
            std::cout << "> ";
            continue;
        }
        
        std::string command = tokens[0];
        
        try {
            if (command == "quit" || command == "exit") {
                std::cout << "Disconnecting..." << std::endl;
                break;
            }
            else if (command == "help") {
                printUsage();
            }
            else if (command == "buy") {
                if (tokens.size() < 4) {
                    std::cout << "Usage: buy <symbol> <quantity> <price>" << std::endl;
                } else {
                    std::string symbol = tokens[1];
                    Quantity quantity = std::stoull(tokens[2]);
                    Price price = doubleToPrice(std::stod(tokens[3]));
                    
                    client.submitOrder(symbol, Side::BUY, OrderType::LIMIT, price, quantity);
                }
            }
            else if (command == "sell") {
                if (tokens.size() < 4) {
                    std::cout << "Usage: sell <symbol> <quantity> <price>" << std::endl;
                } else {
                    std::string symbol = tokens[1];
                    Quantity quantity = std::stoull(tokens[2]);
                    Price price = doubleToPrice(std::stod(tokens[3]));
                    
                    client.submitOrder(symbol, Side::SELL, OrderType::LIMIT, price, quantity);
                }
            }
            else if (command == "market-buy") {
                if (tokens.size() < 3) {
                    std::cout << "Usage: market-buy <symbol> <quantity>" << std::endl;
                } else {
                    std::string symbol = tokens[1];
                    Quantity quantity = std::stoull(tokens[2]);
                    
                    client.submitOrder(symbol, Side::BUY, OrderType::MARKET, 0, quantity);
                }
            }
            else if (command == "market-sell") {
                if (tokens.size() < 3) {
                    std::cout << "Usage: market-sell <symbol> <quantity>" << std::endl;
                } else {
                    std::string symbol = tokens[1];
                    Quantity quantity = std::stoull(tokens[2]);
                    
                    client.submitOrder(symbol, Side::SELL, OrderType::MARKET, 0, quantity);
                }
            }
            else if (command == "cancel") {
                if (tokens.size() < 2) {
                    std::cout << "Usage: cancel <order_id>" << std::endl;
                } else {
                    OrderId orderId = std::stoull(tokens[1]);
                    client.cancelOrder(orderId);
                }
            }
            else if (command == "modify") {
                if (tokens.size() < 4) {
                    std::cout << "Usage: modify <order_id> <price> <quantity>" << std::endl;
                } else {
                    OrderId orderId = std::stoull(tokens[1]);
                    Price price = doubleToPrice(std::stod(tokens[2]));
                    Quantity quantity = std::stoull(tokens[3]);
                    
                    client.modifyOrder(orderId, price, quantity);
                }
            }
            else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
        
        std::cout << "> ";
    }
}

void runDemoMode(Client& client) {
    std::cout << "\nRunning demo mode...\n" << std::endl;
    
    // Wait a bit for connection to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Submit some orders
    std::cout << "Submitting buy orders..." << std::endl;
    client.submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(150.00), 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    client.submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(149.50), 200);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    client.submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(149.00), 150);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "\nSubmitting sell orders..." << std::endl;
    client.submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.00), 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    client.submitOrder("AAPL", Side::SELL, OrderType::LIMIT, doubleToPrice(151.50), 200);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "\nSubmitting matching order (should create trades)..." << std::endl;
    client.submitOrder("AAPL", Side::BUY, OrderType::LIMIT, doubleToPrice(151.50), 150);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nSubmitting market order..." << std::endl;
    client.submitOrder("AAPL", Side::SELL, OrderType::MARKET, 0, 50);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nDemo completed. Press Enter to continue to interactive mode..." << std::endl;
    std::cin.get();
}

int main(int argc, char* argv[]) {
    printWelcome();
    
    // Parse command line arguments
    std::string host = "127.0.0.1";
    uint16_t port = SERVER_PORT;
    bool demoMode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --host <hostname>  Server hostname (default: 127.0.0.1)" << std::endl;
            std::cout << "  --port <port>      Server port (default: " << SERVER_PORT << ")" << std::endl;
            std::cout << "  --demo             Run demo mode" << std::endl;
            return 0;
        }
        else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--demo") {
            demoMode = true;
        }
    }
    
    // Create client
    Client client(host, port);
    
    // Set up callbacks for notifications
    client.setOrderAckCallback([](const OrderAckMessage& msg) {
        // already printed in client
    });
    
    client.setExecutionReportCallback([](const ExecutionReportMessage& msg) {
        // already printed in client
    });
    
    // Connect to server
    std::cout << "\nConnecting to server " << host << ":" << port << "..." << std::endl;
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to server. Is the server running?" << std::endl;
        return 1;
    }
    
    std::cout << "Successfully connected!\n" << std::endl;
    
    // Run in appropriate mode
    if (demoMode) {
        runDemoMode(client);
    }
    
    runInteractiveMode(client);
    
    client.disconnect();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}


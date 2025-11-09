#include "Server.h"
#include "Common.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>

using namespace MatchingEngine;

// Global server instance for signal handling
std::unique_ptr<Server> g_server;

void signalHandler(int signal) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [port]" << std::endl;
    std::cout << "  port: Server port (default: " << SERVER_PORT << ")" << std::endl;
}

void printServerStats(Server* server) {
    while (server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        std::cout << "\n=== Server Statistics ===" << std::endl;
        std::cout << "Active Connections: " << server->getActiveConnections() << std::endl;
        std::cout << "Total Orders: " << server->getTotalOrders() << std::endl;
        std::cout << "Total Trades: " << server->getTotalTrades() << std::endl;
        std::cout << "=========================\n" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Matching Engine Server" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Parse command line arguments
    uint16_t port = SERVER_PORT;
    
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        
        try {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Create and start server
    g_server = std::make_unique<Server>(port);
    
    if (!g_server->start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "\nServer is running. Press Ctrl+C to stop.\n" << std::endl;
    
    // Start stats thread
    std::thread statsThread(printServerStats, g_server.get());
    
    // Keep main thread alive
    while (g_server->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    if (statsThread.joinable()) {
        statsThread.join();
    }
    
    return 0;
}


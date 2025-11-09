# Matching Engine

A high-performance, professional-grade limit order book matching engine implemented in C++ with full client-server architecture.

## Features

### Order Types
- **Limit Orders**: Execute at specified price or better
- **Market Orders**: Execute immediately at best available price
- **Stop Loss**: Becomes market order when stop price is reached
- **Stop Limit**: Becomes limit order when stop price is reached
- **IOC (Immediate or Cancel)**: Execute immediately, cancel unfilled portion
- **FOK (Fill or Kill)**: Execute completely or cancel entirely

### Order Operations
- **Submit**: Add new orders to the order book
- **Cancel**: Remove pending orders
- **Modify**: Change price and quantity (loses time priority)

### Matching Logic
- Price-time priority matching
- Aggressive orders matched against passive orders
- Proper handling of partial fills
- Real-time trade execution and reporting

### Architecture
- **Client-Server Separation**: Full network-based communication
- **Thread-Safe**: Concurrent order processing with mutex protection
- **Scalable Design**: Efficient data structures for high throughput
- **Message Protocol**: Binary protocol for low-latency communication

## Project Structure

```
matching-engine-cpp/
├── include/              # Header files
│   ├── Common.h         # Common types and constants
│   ├── Order.h          # Order class
│   ├── Trade.h          # Trade class
│   ├── OrderBook.h      # Order book implementation
│   ├── MatchingEngine.h # Matching engine core
│   ├── Message.h        # Network message protocol
│   ├── Server.h         # Server implementation
│   └── Client.h         # Client implementation
├── src/                 # Implementation files
│   ├── Order.cpp
│   ├── Trade.cpp
│   ├── OrderBook.cpp
│   ├── MatchingEngine.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── main_server.cpp  # Server entry point
│   └── main_client.cpp  # Client entry point
└── CMakeLists.txt       # Build configuration
```

## Building

### Prerequisites
- CMake 3.12 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- Windows: Visual Studio 2017+ or MinGW
- Linux/Unix: GCC 7+ or Clang 5+

### Build Instructions

#### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Linux/Unix
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

The executables will be in the `build/` directory (or `build/Release/` on Windows):
- `matching_server` - The matching engine server
- `matching_client` - The client application

## Usage

### Starting the Server

```bash
# Use default port (8888)
./matching_server

# Specify custom port
./matching_server 9999
```

The server will:
- Listen for client connections
- Process orders and execute matches
- Print trade executions and statistics
- Show active connections and order/trade counts every 10 seconds

### Running the Client

#### Interactive Mode
```bash
# Connect to local server
./matching_client

# Connect to remote server
./matching_client --host 192.168.1.100 --port 8888
```

#### Demo Mode
```bash
# Run demo with sample orders
./matching_client --demo
```

### Client Commands

```
buy <symbol> <quantity> <price>       - Submit a buy limit order
sell <symbol> <quantity> <price>      - Submit a sell limit order
market-buy <symbol> <quantity>        - Submit a market buy order
market-sell <symbol> <quantity>       - Submit a market sell order
cancel <order_id>                     - Cancel an order
modify <order_id> <price> <quantity>  - Modify an order
help                                  - Show help message
quit                                  - Disconnect and exit
```

### Example Session

```bash
> buy AAPL 100 150.00
[CLIENT] Order sent: AAPL BUY 100 @ 150.0000
[CLIENT] Order ACK: Client Order 1 -> Server Order 1 Status: PENDING

> sell AAPL 50 150.00
[CLIENT] Order sent: AAPL SELL 50 @ 150.0000
[CLIENT] Order ACK: Client Order 2 -> Server Order 2 Status: PARTIAL_FILL
[CLIENT] Execution Report: Order 2 Symbol: AAPL Side: SELL Executed: 50 @ 150.0000

> cancel 1
[CLIENT] Cancel order sent: 1
[CLIENT] Order ACK: Order 1 Status: CANCELLED
```

## Technical Details

### Order Book Implementation

The order book uses efficient data structures for fast matching:
- **Price Levels**: `std::map` for sorted price levels (O(log n) insertion)
- **Order Queue**: `std::list` for FIFO queue at each price level
- **Order Lookup**: `std::unordered_map` for O(1) order lookup
- **Thread Safety**: `std::mutex` for concurrent access protection

### Matching Algorithm

1. **Price-Time Priority**: Orders matched first by best price, then by arrival time
2. **Aggressive Matching**: Incoming orders match against resting orders
3. **Partial Fills**: Orders can be partially filled across multiple price levels
4. **Price Improvement**: Trades execute at the passive order's price

### Network Protocol

Binary message protocol for efficiency:
- Fixed-size messages for predictable parsing
- Message header with type and length
- Support for all order operations and callbacks
- Heartbeat support for connection monitoring

### Performance Characteristics

- **Order Submission**: O(log n) where n is number of price levels
- **Order Cancellation**: O(log n) + O(1) lookup
- **Order Modification**: O(log n) for removal + O(log n) for insertion
- **Matching**: O(m) where m is number of orders matched
- **Memory**: Linear with number of active orders

## Design Patterns

- **Separation of Concerns**: Clear separation between order book, matching engine, and network layers
- **Callbacks**: Observer pattern for order and trade notifications
- **Resource Management**: RAII for socket and thread management
- **Type Safety**: Strong typing with type aliases for clarity

## Future Enhancements

Potential improvements for production use:
- Persistent storage (database or file-based)
- Order book snapshots and recovery
- Multiple matching engines for different symbols
- Market data distribution
- Risk management and position tracking
- FIX protocol support
- Performance metrics and monitoring
- Unit tests and integration tests

## License

This is a demonstration project for educational purposes.

## Author

Built as a professional example of a matching engine implementation in C++.


# Matching Engine

A limit order book matching engine in C++ with client-server architecture. supports the usual order types (market, limit, IOC, FOK, stop orders), proper matching logic with price-time priority, and handles partial fills.

## Quick Start

Build it:
```bash
./build.sh      # linux/mac
build.bat       # windows
```

Run the server:
```bash
./build/matching_server
```

Connect a client (in another terminal):
```bash
./build/matching_client

# or try demo mode
./build/matching_client --demo
```

## Using the Client

```bash
> buy AAPL 100 150.00        # limit order
> sell AAPL 50 151.00
> market-buy AAPL 20         # market order
> cancel 1                   # cancel order
> modify 3 149.50 200        # modify existing order
> quit
```

## What's Inside

**Order types:**
- Limit (execute at price or better)
- Market (execute immediately)
- IOC (immediate or cancel)
- FOK (fill or kill)
- Stop loss / stop limit

**Features:**
- Price-time priority matching
- Partial fills across multiple price levels
- Thread-safe for concurrent clients
- Binary network protocol for low latency
- Real-time execution reports

## How It Works

The order book uses `std::map` for price levels (keeps them sorted), `std::list` for FIFO queues at each price, and `std::unordered_map` for fast order lookups. Matching is O(log n) for submission and O(m) for executing m orders.

When you submit an order:
1. If it crosses the spread, it matches against existing orders
2. Trades execute at the passive (resting) order's price
3. Partial fills work across multiple price levels
4. Any remaining quantity goes into the book

## Structure

```
include/          headers for everything
src/             implementations
  main_server.cpp   server entry point
  main_client.cpp   client with interactive CLI
```

Core components: Order, OrderBook, MatchingEngine, plus Server/Client for networking.

## Requirements

- C++17 compiler
- CMake 3.12+
- Works on Windows, Linux, Mac


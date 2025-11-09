# Quick Start Guide

Get up and running with the Matching Engine in 5 minutes!

## 1. Build the Project

### Linux/Mac:
```bash
chmod +x build.sh
./build.sh
```

### Windows:
```cmd
build.bat
```

## 2. Start the Server

### Linux/Mac:
```bash
./build/matching_server
```

### Windows:
```cmd
build\Release\matching_server.exe
```

**Expected Output:**
```
========================================
  Matching Engine Server
========================================
Server started on port 8888

Server is running. Press Ctrl+C to stop.
```

## 3. Run the Client

Open a new terminal/command prompt:

### Linux/Mac:
```bash
./build/matching_client
```

### Windows:
```cmd
build\Release\matching_client.exe
```

**Expected Output:**
```
========================================
  Matching Engine Client
========================================

Connecting to server 127.0.0.1:8888...
Connected to server 127.0.0.1:8888
Successfully connected!
```

## 4. Try Some Commands

```bash
# Submit a buy order
> buy AAPL 100 150.00

# Submit a sell order
> sell AAPL 50 151.00

# Submit a matching order (creates a trade!)
> buy AAPL 30 151.00

# Market order
> market-buy AAPL 20

# Cancel an order
> cancel 1

# Quit
> quit
```

## 5. Run Demo Mode

To see a pre-programmed demonstration:

### Linux/Mac:
```bash
./build/matching_client --demo
```

### Windows:
```cmd
build\Release\matching_client.exe --demo
```

This will automatically submit various orders and show you how matching works!

## Key Concepts

### Order Types
- **LIMIT**: Execute at specified price or better
- **MARKET**: Execute immediately at best price
- **IOC**: Immediate or Cancel
- **FOK**: Fill or Kill

### Order Operations
- **Submit**: Add new order
- **Cancel**: Remove pending order
- **Modify**: Change price/quantity (loses time priority)

### Matching Rules
- **Price Priority**: Best price first
- **Time Priority**: Earlier orders first at same price
- **Passive Price**: Trades execute at resting order price

## Architecture

```
Client Application ←→ [Network] ←→ Server
                                    ↓
                              Matching Engine
                                    ↓
                               Order Book(s)
```

## File Structure

```
include/          - Header files (.h)
src/             - Implementation files (.cpp)
build/           - Compiled executables
README.md        - Full documentation
EXAMPLES.md      - Detailed examples
```

## Common Issues

### Port Already in Use
```bash
# Change the port
./matching_server 9999
./matching_client --port 9999
```

### Connection Refused
- Make sure the server is running first
- Check firewall settings
- Verify correct IP/port

### Build Errors
```bash
# Clean and rebuild
rm -rf build
./build.sh
```

## Next Steps

1. Read [README.md](README.md) for detailed documentation
2. Check [EXAMPLES.md](EXAMPLES.md) for usage patterns
3. Experiment with different order types
4. Try multiple clients simultaneously
5. Modify the code for your needs!

## Quick Command Reference

### Server
```bash
matching_server [port]              # Start server
matching_server --help              # Show help
```

### Client
```bash
matching_client                     # Connect to localhost:8888
matching_client --host <ip>         # Connect to specific host
matching_client --port <port>       # Connect to specific port
matching_client --demo              # Run demo mode
```

### Client Commands
```
buy <symbol> <qty> <price>          # Limit buy
sell <symbol> <qty> <price>         # Limit sell
market-buy <symbol> <qty>           # Market buy
market-sell <symbol> <qty>          # Market sell
cancel <order_id>                   # Cancel order
modify <order_id> <price> <qty>     # Modify order
help                                # Show help
quit                                # Exit
```

## Performance Tips

- The engine uses efficient data structures (O(log n) operations)
- Thread-safe for concurrent client connections
- Binary protocol for low latency
- Price-time priority for fair matching

## Support

For issues or questions:
1. Check the README.md and EXAMPLES.md
2. Review the code comments
3. Test with the demo mode
4. Verify server is running and accessible

---

**Happy Trading!** 🚀


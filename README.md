# PubSub Distributed Messaging System

A distributed publish-subscribe messaging system with leader election, built in C++.

## Features

- **Distributed Broker Cluster**: Multiple brokers with Bully algorithm leader election
- **Concurrent Message Distribution**: Leader forwards messages to all brokers for parallel processing
- **Message Persistence**: SQLite-based message storage with catchup support
- **Real-time Stock Data**: Publisher fetches live stock prices from Polygon.io API
- **Auto-reconnection**: Clients automatically reconnect to the current leader
- **Configuration System**: Environment-based configuration with sensible defaults

## Instructions

### 1. Install Dependencies 

```bash
make install-deps
```
### 2. Set Up Environment

Copy the example environment file and configure:
```bash
cp .env.example .env
# Need Polygon.io API key
```

Get a free API key from [Polygon.io](https://polygon.io/)

### 3. Start the System

```bash
make start
```

This will:
- Build all components
- Start the broker cluster
- Perform leader election
- Show available commands

### 4. Use the System

In separate terminals:

**Start Publisher:**
```bash
./publisher
```

**Start Subscriber:**
```bash
./subscriber [topic]
# Example: ./subscriber AAPL
```

## Build Options

```bash
make help           # Show all available targets
make all            # Build all components 
make debug          # Build with debug symbols
make release        # Build optimized version
make clean          # Clean build artifacts
make test           # Run test scenario
```

## Configuration

Set environment variables or use `.env` file:

- `POLYGON_API_KEY`: Your Polygon.io API key (required for publisher)
- `BROKER_HOST`: Broker host address (default: 127.0.0.1)
- `BROKER_PORTS`: Comma-separated broker ports (default: 8080,8081,8082)

## Architecture

### Components

1. **Brokers (`election`)**: Distributed message brokers with leader election
2. **Publisher (`publisher`)**: Publishes real-time stock data
3. **Subscriber (`subscriber`)**: Subscribes to topics and receives messages

### Message Flow

1. Brokers start and elect a leader using consensus protocol
2. Publisher connects to leader and sends stock data
3. Leader forwards messages to other brokers for concurrent distribution
4. Subscribers connect to any broker and subscribe to topics
5. Messages are persisted and distributed concurrently across all brokers
6. On leader failure, new leader is elected automatically


### File Structure

```
src/
├── config/           # Configuration system
├── broker/           # Broker implementation
├── publisher/        # Publisher implementation
└── subscriber/       # Subscriber implementation
```


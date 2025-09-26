#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

POLYGON_API_KEY_DEFAULT="YOUR_API_KEY_HERE"
BROKER_HOST_DEFAULT="127.0.0.1"
BROKER_PORTS_DEFAULT="8080,8081,8082"

echo -e "${BLUE}Distributed PubSub System Startup ${NC}"

if [ -z "$POLYGON_API_KEY" ]; then
    echo -e "${YELLOW}POLYGON_API_KEY environment variable not set${NC}"
    export POLYGON_API_KEY="$POLYGON_API_KEY_DEFAULT"
fi

export BROKER_HOST="${BROKER_HOST:-$BROKER_HOST_DEFAULT}"
export BROKER_PORTS="${BROKER_PORTS:-$BROKER_PORTS_DEFAULT}"

echo -e "${BLUE}Configuration:${NC}"
echo "  BROKER_HOST: $BROKER_HOST"
echo "  BROKER_PORTS: $BROKER_PORTS"


echo -e "${BLUE}Building system${NC}"
if ! make clean && make all; then
    echo -e "${RED}Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful${NC}"

start_broker() {
    local port=$1
    echo -e "${BLUE}Starting broker on port $port${NC}"
    ./election $port &
    sleep 2
}

cleanup() {
    echo -e "${YELLOW}Shutting down system${NC}"
    pkill -f "election" || true
    pkill -f "publisher" || true
    pkill -f "subscriber" || true
    echo -e "${GREEN}System shutdown complete${NC}"
}

trap cleanup EXIT INT TERM

IFS=',' read -ra PORTS <<< "$BROKER_PORTS"
for port in "${PORTS[@]}"; do
    start_broker "$port"
done

echo -e "${BLUE}Waiting for leader election${NC}"
sleep 5

echo -e "${GREEN}System started${NC}"
echo -e "${BLUE}Available commands${NC}"
echo "  Start publisher: ./publisher"
echo "  Start subscriber: ./subscriber [topic]"
echo "  Stop system: Ctrl+C"

echo -e "${YELLOW}Press Ctrl+C to stop the system${NC}"
while true; do
    sleep 1
done
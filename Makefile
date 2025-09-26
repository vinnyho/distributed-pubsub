CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -pthread -Isrc -fstack-protector-strong -D_FORTIFY_SOURCE=2
LIBS_BROKER = -lsqlite3
LIBS_PUBLISHER = -lcurl

SRCDIR = src
CONFIGDIR = $(SRCDIR)/config
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj

CONFIG_SOURCES = $(CONFIGDIR)/config.cpp

BROKER_SOURCES = $(SRCDIR)/broker/broker.cpp $(SRCDIR)/broker/helpers/message_store/message_store.cpp $(CONFIG_SOURCES)
PUBLISHER_SOURCES = $(SRCDIR)/publisher/publisher.cpp $(SRCDIR)/publisher/publisher_apis/stock_fetcher.cpp $(SRCDIR)/publisher/publisher_apis/request.cpp $(CONFIG_SOURCES)
SUBSCRIBER_SOURCES = $(SRCDIR)/subscriber/subscriber.cpp $(CONFIG_SOURCES)
ELECTION_SOURCES = $(SRCDIR)/broker/helpers/broker_starter/broker_starter.cpp $(SRCDIR)/broker/broker.cpp $(SRCDIR)/broker/helpers/message_store/message_store.cpp $(CONFIG_SOURCES)

CONFIG_HEADERS = $(CONFIGDIR)/config.h


TARGETS = publisher subscriber election


all: create-dirs $(TARGETS)


create-dirs:
	@mkdir -p $(BUILDDIR) $(OBJDIR)


publisher: $(PUBLISHER_SOURCES) $(CONFIG_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(PUBLISHER_SOURCES) $(LIBS_PUBLISHER)


subscriber: $(SUBSCRIBER_SOURCES) $(CONFIG_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SUBSCRIBER_SOURCES)


election: $(ELECTION_SOURCES) $(CONFIG_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(ELECTION_SOURCES) $(LIBS_BROKER)


debug: CXXFLAGS += -g -DDEBUG -O0
debug: all

release: CXXFLAGS += -O2 -DNDEBUG
release: all


start: all
	./start_system.sh


test: all
	@echo "Starting test scenario..."
	@echo "Set POLYGON_API_KEY environment variable first!"
	./start_system.sh &
	sleep 5
	@echo "System is running. Press Ctrl+C to stop."

#
clean:
	rm -f $(TARGETS) *.db *.log
	rm -rf $(BUILDDIR)


clean-all: clean
	rm -f .env


install-deps:
	@echo "Installing dependencies..."
	@command -v brew >/dev/null 2>&1 || { echo "Homebrew is required but not installed." >&2; exit 1; }
	brew install sqlite3 curl


help:
	@echo "Available targets:"
	@echo "  all          - Build all executables (default)"
	@echo "  debug        - Build with debug symbols and no optimization"
	@echo "  release      - Build optimized release version"
	@echo "  start        - Build and start the system"
	@echo "  test         - Run test scenario"
	@echo "  clean        - Remove build artifacts and databases"
	@echo "  clean-all    - Remove everything including .env"
	@echo "  install-deps - Install required dependencies (macOS)"
	@echo "  help         - Show this help message"

.PHONY: all create-dirs debug release start test clean clean-all install-deps help
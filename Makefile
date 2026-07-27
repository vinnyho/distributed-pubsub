CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -pthread

ifeq ($(SANITIZE),1)
	CXXFLAGS += -fsanitize=thread
endif

BROKER_SRCS := src/broker/main.cpp src/broker/broker.cpp

broker: $(BROKER_SRCS)
	$(CXX) $(CXXFLAGS) -o broker $(BROKER_SRCS)

clean:
	rm -f broker broker_nosan
	rm -rf *.dSYM

.PHONY: clean

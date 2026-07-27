CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -pthread

ifeq ($(SANITIZE),1)
	CXXFLAGS += -fsanitize=thread
endif

broker: src/broker/main.cpp
	$(CXX) $(CXXFLAGS) -o broker src/broker/main.cpp

clean:
	rm -f broker broker_nosan
	rm -rf *.dSYM

.PHONY: clean

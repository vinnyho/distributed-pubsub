CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -pthread -Isrc

ifeq ($(SANITIZE),1)
	CXXFLAGS += -fsanitize=thread
endif

FRAME_SRC := src/framing/frame.cpp
BROKER_SRCS := src/broker/main.cpp src/broker/broker.cpp $(FRAME_SRC)

broker: $(BROKER_SRCS)
	$(CXX) $(CXXFLAGS) -o broker $(BROKER_SRCS)

frame_client: tools/frame_echo_client.cpp $(FRAME_SRC)
	$(CXX) $(CXXFLAGS) -o frame_client tools/frame_echo_client.cpp $(FRAME_SRC)

clean:
	rm -f broker frame_client broker_nosan
	rm -rf *.dSYM

.PHONY: clean broker frame_client

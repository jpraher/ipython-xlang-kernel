
CXXFLAGS+=-g

.PHONY: all

all: kernel json_test
	echo "${CXXFLAGS}"

kernel: kernel.h kernel.o json.o ipython_message.o hmac.o message.o tthread/tinythread.o
	$(CXX) ${CXXFLAGS} -lssl -lcrypto -lzmq -lglog kernel.o json.o ipython_message.o message.o hmac.o tthread/tinythread.o -o kernel


json_test: json.h json.cpp json_test.cpp
	$(CXX) ${CXXFLAGS} -lglog json.cpp json_test.cpp -o json_test

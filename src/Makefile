
CXXFLAGS+=-g

.PHONY: all

all: kernel json_test
	echo "${CXXFLAGS}"

kernel: kernel.h kernel.o json.o ipython_message.o
	$(CXX) ${CXXFLAGS} -lzmq kernel.o json.o ipython_message.o -o kernel


json_test: json.h json.cpp json_test.cpp
	$(CXX) ${CXXFLAGS} json.cpp json_test.cpp -o json_test

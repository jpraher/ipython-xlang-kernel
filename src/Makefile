

kernel: kernel.h kernel.o
	$(CXX) -g -lzmq kernel.o -o kernel


json_test: json.h json.cpp json_test.cpp
	$(CXX) -g json.cpp json_test.cpp -o json_test

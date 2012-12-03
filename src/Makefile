
CXXFLAGS+=-g -O0

.PHONY: all

#  redir_test
all: kernel json_test
	echo "${CXXFLAGS}"

kernel: kernel.h kernel.o json.o ipython_message.o hmac.o message.o tthread/tinythread.o ioredir.o delegate.h
	$(CXX) ${CXXFLAGS}  -lssl -lcrypto -lzmq -lglog kernel.o json.o ioredir.o ipython_message.o message.o hmac.o tthread/tinythread.o delegate.h -o kernel


json_test: json.h json.cpp json_test.cpp
	$(CXX) ${CXXFLAGS} -lglog json.cpp json_test.cpp -o json_test


# redir_test: json.h ioredir.cpp tthread/tinythread.o json.o delegate.h
#	$(CXX) ${CXXFLAGS} -lglog  -lzmq ioredir.cpp tthread/tinythread.o json.o -o redir_test

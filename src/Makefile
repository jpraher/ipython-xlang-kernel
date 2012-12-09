
CXXFLAGS+=-g -O0
SHLIB_FLAGS=-dynamiclib
SHLIB_EXT=.dylib

.PHONY: all

#  delegate.h
#  redir_test

all: libipython-xlang-kernel${SHLIB_EXT} kernel json_test
	echo "${CXXFLAGS}"


libipython-xlang-kernel${SHLIB_EXT}: kernel.h kernel.o json.o ipython_message.o hmac.o message.o tthread/tinythread.o ioredir.o delegate.h ipython_shell_handler.h ipython_shell_handler.o
	$(CXX) ${CXXFLAGS} ${SHLIB_FLAGS} -lssl -lcrypto -lzmq -lglog kernel.o json.o ipython_message.o hmac.o message.o ipython_shell_handler.o tthread/tinythread.o ioredir.o \
	-o libipython-xlang-kernel${SHLIB_EXT}


kernel: kernel.h kernel_main.o libipython-xlang-kernel${SHLIB_EXT}
	$(CXX) ${CXXFLAGS} -L. -lssl -lcrypto -lzmq -lglog -lipython-xlang-kernel kernel_main.o  delegate.h -o kernel



json_test: json.h json.cpp json_test.cpp
	$(CXX) ${CXXFLAGS} -lglog json.cpp json_test.cpp -o json_test


# redir_test: json.h ioredir.cpp tthread/tinythread.o json.o delegate.h
#	$(CXX) ${CXXFLAGS} -lglog  -lzmq ioredir.cpp tthread/tinythread.o json.o -o redir_test

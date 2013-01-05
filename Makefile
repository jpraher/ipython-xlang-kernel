
CXXFLAGS+=-g -O0
SHLIB_FLAGS=-dynamiclib
SHLIB_EXT=.dylib

.PHONY: all clean

all: libipython-xlang-kernel${SHLIB_EXT} kernel json_test ioredir_test
	echo "${CXXFLAGS}"


libipython-xlang-kernel${SHLIB_EXT}: kernel.h kernel.o json.o ipython_message.o hmac.o message.o tthread/tinythread.o ioredir.o delegate.h ipython_shell_handler.h ipython_shell_handler.o api.o
	$(CXX) ${CXXFLAGS} ${SHLIB_FLAGS} -lssl -lcrypto -lzmq -lglog api.o kernel.o json.o ipython_message.o hmac.o message.o ipython_shell_handler.o tthread/tinythread.o ioredir.o \
	-o libipython-xlang-kernel${SHLIB_EXT}


kernel: kernel.h kernel_main.o libipython-xlang-kernel${SHLIB_EXT}
	$(CXX) ${CXXFLAGS} -L. -lssl -lcrypto -lzmq -lglog -lipython-xlang-kernel kernel_main.o  delegate.h -o kernel


kernel2: api.h kernel2_main.o libipython-xlang-kernel${SHLIB_EXT}
	$(CC) ${CFLAGS} -L. -lssl -lcrypto -lzmq -lglog -lipython-xlang-kernel kernel2_main.o  -o kernel2


json_test: json.h json.cpp json_test.cpp
	$(CXX) ${CXXFLAGS} -lglog $^ -o $@

ioredir_test: ioredir_test.cpp api.h ioredir.h libipython-xlang-kernel${SHLIB_EXT}
	$(CXX) ${CXXFLAGS} -L. -lglog -lipython-xlang-kernel -lzmq $< -o $@

clean:
	rm -f libipython-xlang-kernel${SHLIB_EXT}
	rm -f *.o
	rm -f tthread/*.o
	rm -f json_test
	rm -f ioredir_test
	rm -f kernel kernel2


/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __message__h__
#define __message__h__

#include "json.h"
#include <zmq.hpp>
#include <sstream>

class Message {
public:
    virtual bool serialize(const std::string & key, std::list<zmq::message_t*> *messages) const = 0;
    virtual bool deserialize(const std::list<zmq::message_t*> & messages) = 0;

    static void free(std::list<zmq::message_t*> & messages);
};


class Channel {
public:
    virtual ~Channel() {}
    virtual void send(const Message& message) = 0;
};

class EContext {
public:
    EContext(Channel &io, Channel &shell);

    Channel & io();
    Channel & shell();

    void handle_stdout(const std::string &s);
    void handle_stderr(const std::string &s);

    std::string stdout();
    std::string stderr();

private:
    Channel * _io;
    Channel * _shell;
    std::ostringstream _stdout_oss;
    std::ostringstream _stderr_oss;
};

class ExecuteHandler {
public:

    virtual Message * create_request() = 0;

    virtual void execute(EContext & ctx,
                         Message * request) = 0;
};



#endif // __ipython__message__h__

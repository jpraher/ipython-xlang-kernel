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
#include "ioredir.h"
#include <zmq.hpp>
#include <sstream>
#include <uuid/uuid.h>


std::string _generate_uuid();

std::ostream & _uuid_stringify(const uuid_t & uuid,
                               std::ostream & os);

std::istream & _uuid_parse(std::istream &is,
                           uuid_t &uuid);

class Message {
public:
    virtual bool serialize(const std::string & key,
                           std::list<zmq::message_t*> *messages) const = 0;
    virtual bool deserialize(const std::list<zmq::message_t*> & messages) = 0;

    static void free(std::list<zmq::message_t*> & messages);
};

class Session {
};

class Channel {
public:
    virtual ~Channel() {}
    virtual void send(const Message& message) = 0;
};

class EContext {
public:
    EContext(const std::string & ident,
             Channel &io,
             Channel &shell,
             redirector & stderr_redir,
             redirector & stdout_redir);

    Channel & io();
    Channel & shell();

    typedef std::string (EContext::*get_string_t)();

    const std::string & ident() const;
    const std::string & session_id()  const;

    std::string stdout();
    std::string stderr();

private:
    std::string _session_id;
    std::string _ident;
    Channel * _io;
    Channel * _shell;
    redirector *_stdout_redir;
    redirector *_stderr_redir;
};

class ExecuteHandler {
public:

    virtual Message * create_request() = 0;

    virtual void execute(EContext & ctx,
                         Message * request) = 0;
};



#endif // __ipython__message__h__

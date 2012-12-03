/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "message.h"

void Message::free(std::list<zmq::message_t*> & messages) {
    for (std::list<zmq::message_t*>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        delete *it;
        *it = NULL;
    }
}

EContext::EContext(Channel & io,
                   Channel & shell)
    : _io(&io),
      _shell(&shell)
{
}

void EContext::handle_stdout(const std::string &s) {
    _stdout_oss << s;
}
void EContext::handle_stderr(const std::string &s) {
    _stderr_oss << s;
}

std::string EContext::stdout() {
    std::string s = _stdout_oss.str();
    _stdout_oss.str("");
    return s;
}

std::string EContext::stderr() {
    std::string s = _stderr_oss.str();
    _stderr_oss.str("");
    return s;
}


Channel & EContext::io() {
    assert(_io);
    return *_io;
}

Channel & EContext::shell() {
    assert(_shell);
    return *_shell;
}

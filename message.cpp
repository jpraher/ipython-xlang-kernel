/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "message.h"
#include <uuid/uuid.h>


std::string _generate_uuid() {
    uuid_t id;
    uuid_generate(id);
    std::ostringstream oss;
    _uuid_stringify(id, oss);
    return oss.str();
}
std::ostream & _uuid_stringify(const uuid_t & uuid,
                               std::ostream & os) {

    assert(sizeof(uuid) == 16);

    for (size_t i = 0; i < sizeof(uuid); ++i ) {
        int c = uuid[i];
        char oldfill = os.fill('0');
        std::streamsize oldwith = os.width(2);
        os << std::hex << c;
        os.fill(oldfill);
        os.width(oldwith);
        if (i == 3 || i == 5 || i ==  7 || i == 9) {
            os << "-";
        }
    }
}


int _hex_decode(int n) {
    if (n >= '0' && n <= '9') {
        return n - '0';
    }
    if (n >= 'a' && n <= 'f') {
        return (n - 'a') + 10;
    }
    if (n >= 'A' && n <= 'F') {
        return (n - 'a') + 10;
    }
    assert (false && "overflow");
}

std::istream & _uuid_parse(std::istream &is,
               uuid_t &uuid)
{
    //
    int i = 0;
    do {
        // one byte
        int n1 = is.get();
        if (n1 == '-') continue;
        int n2 = is.get();

        n1 = _hex_decode(n1);
        n2 = _hex_decode(n2);
        int n =  n1 << 4 | (n2 & 0xF);
        uuid[i++] = (char)n;
    } while (i < 16);

}


void Message::free(std::list<zmq::message_t*> & messages) {
    for (std::list<zmq::message_t*>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        delete *it;
        *it = NULL;
    }
}

EContext::EContext(const std::string & ident,
                   Channel & io,
                   Channel & shell,
                   Channel & in,
                   redirector & stdout_redirector,
                   redirector & stderr_redirector)
    : _session_id(_generate_uuid()),
      _ident(ident),
      _io(&io),
      _in(&in),
      _shell(&shell),
      _stdout_redir(&stdout_redirector),
      _stderr_redir(&stderr_redirector)
{
}

std::string EContext::stdout() {
    std::string s;
    if (_stdout_redir) {
        bool result = _stdout_redir->receive(s);
    }
    return s;
}

std::string EContext::stderr() {
    std::string s;
    if (_stderr_redir) {
        bool result = _stderr_redir->receive(s);
    }
    return s;
}

const std::string & EContext::session_id() const {
    return _session_id;
}
const std::string & EContext::ident() const {
    return _ident;
}

Channel & EContext::io() {
    assert(_io);
    return *_io;
}

Channel & EContext::shell() {
    assert(_shell);
    return *_shell;
}

Channel & EContext::in() {
    assert(_in);
    return *_in;
}

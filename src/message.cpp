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
                   Channel & shell)
    : _session_id(_generate_uuid()),
      _ident(ident),
      _io(&io),
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

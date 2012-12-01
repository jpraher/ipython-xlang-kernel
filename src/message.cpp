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

Channel & EContext::io() {
    assert(_io);
    return *_io;
}

Channel & EContext::shell() {
    assert(_shell);
    return *_shell;
}

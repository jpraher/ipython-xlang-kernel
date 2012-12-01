
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


#ifndef __ipython__message__h__
#define __ipython__message__h__

#include "json.h"
#include <zmq.hpp>

class IPythonMessage {
public:
    static const char * DELIM;

    std::string session_id;
    std::string hmac;
    json::object_value header;
    json::object_value parent;
    json::object_value metadata;
    json::object_value content;

    bool serialize(std::list<zmq::message_t*> *messages) const;
    bool deserialize(const std::list<zmq::message_t*> & messages);

};


#endif // __ipython__message__h__

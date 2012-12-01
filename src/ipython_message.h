/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __ipython__message__h__
#define __ipython__message__h__

#include "json.h"
#include <zmq.hpp>
#include "message.h"


class IPythonMessage : public Message {
public:
    static const char * DELIM;

    virtual ~IPythonMessage() ;

    std::string session_id;
    std::string hmac;
    json::object_value header;
    json::object_value parent;
    json::object_value metadata;
    json::object_value content;

    // serialization, deserialization
    virtual bool serialize(const std::string & key, std::list<zmq::message_t*> *messages) const;
    virtual bool deserialize(const std::list<zmq::message_t*> & messages);

};


#endif // __ipython__message__h__

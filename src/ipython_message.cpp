/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "ipython_message.h"
#include "hmac.h"
#include <sstream>
#include <glog/logging.h>


// IPythonMessage::IPythonMessage() {
// }

const char * IPythonMessage::DELIM = "<IDS|MSG>";

zmq::message_t * convert(const std::string & s) {
    char *buf = new char[s.size()];
    memcpy(buf, s.data(), s.size());
    return new zmq::message_t((void*)buf, s.size(), NULL);
}

IPythonMessage::~IPythonMessage() {
}

bool IPythonMessage::serialize(const std::string & key, std::list<zmq::message_t*> *messages) const {

    md::hmac_ctx_t ctx;
    md::HMAC hmac_(ctx, key);

    {
        std::ostringstream os;
        header.stringify(os);
        std::string data = os.str();
        messages->push_back(convert(data));
        hmac_.update(data);
    }
    {
        std::ostringstream os;
        parent.stringify(os);
        std::string data = os.str();
        messages->push_back(convert(data));
        hmac_.update(data);
    }
    {
        std::ostringstream os;
        metadata.stringify(os);
        std::string data = os.str();
        messages->push_back(convert(data));
        hmac_.update(data);
    }
    {
        std::ostringstream os;
        content.stringify(os);
        std::string data = os.str();
        messages->push_back(convert(data));
        hmac_.update(data);
    }

    {
        std::string hmac_md;
        std::ostringstream oss;
        hmac_.final(&hmac_md);
        oss << std::hex;
        for (size_t i = 0; i < hmac_md.size(); i++) {
            oss.fill('0'); oss.width(2);
            oss << (0xff & (unsigned int)hmac_md[i]);
        }
        DLOG(INFO) << "hmac hex encoded " << oss.str();
        messages->push_front(convert(oss.str()));
    }

    messages->push_front(convert(DELIM));
    messages->push_front(convert(session_id));
}


// to_send: ['<IDS|MSG>', 'b3f7486530469f95104f5d08b671ab49', '{"username":"jakob","msg_type":"execute_request","msg_id":"cbf262c7-81ff-4c0d-824f-8a6ccdb6d35b","version":[0,14,0,"dev"],"session":"8031b811-0d72-4a5f-96ab-83867665cc96","date":"2012-12-03T22:43:28.182231"}', '{}', '{}', '{"user_variables":[],"code":"1\\n","silent":false,"allow_stdin":true,"store_history":true,"user_expressions":{}}']
bool IPythonMessage::deserialize(const std::list<zmq::message_t*> & messages) {

    std::list<zmq::message_t*>::const_iterator msg = messages.begin();
    if (msg == messages.end()) return false;

    session_id = std::string((char*)(*msg)->data(), (*msg)->size());
    DLOG(INFO) << "session_id " << session_id;

    bool noDelim = true;
    do {
        msg++;
        if (msg == messages.end()) return false;

        noDelim = (*msg)->size() != 9 || memcmp((void*)DELIM, (*msg)->data(), 9) != 0;
        if (noDelim) {
            // TODO: track IDs
        }

    } while(noDelim);

    msg++;
    if (msg == messages.end()) return false;
    hmac = std::string((char*)(*msg)->data(), (*msg)->size());

    msg++;
    if (msg == messages.end()) return false;
    {
        std::string s((char*)(*msg)->data(), (*msg)->size());
        std::istringstream is(s);
        json::parser parser(is);
        parser.parse(&header);
    }
    msg++;
    if (msg == messages.end()) return false;
    {
        std::string s((char*)(*msg)->data(), (*msg)->size());
        std::istringstream is(s);
        json::parser parser(is);
        parser.parse(&parent);
    }
    msg++;
    if (msg == messages.end()) return false;
    {
        std::string s((char*)(*msg)->data(), (*msg)->size());
        std::istringstream is(s);
        json::parser parser(is);
        parser.parse(&metadata);
    }
    msg++;
    if (msg == messages.end()) return false;
    {
        std::string s((char*)(*msg)->data(), (*msg)->size());
        std::istringstream is(s);
        json::parser parser(is);
        parser.parse(&content);
    }
    msg++;
    return msg == messages.end();
}

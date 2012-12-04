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

/*
idents ['\x00\xdb|R\xe6\xea\x00H\xc4\x90tgsE\x14\x92\xd1']
to_send: ['kernel.43636566-0f3c-42f2-88af-99991797eac0.status', '<IDS|MSG>', 'ef6b031f1b2c57a5ece01900f8be7b68', '{"username":"kernel","msg_type":"status","msg_id":"5e14e59f-a2d7-47e9-9435-66fe13194df8","version":[0,14,0,"dev"],"session":"c0bf1dc7-0d1d-4c9d-8cdf-cbc004f22d0a","date":"2012-12-04T00:17:29.264680"}', '{"username":"username","msg_id":"139ED717ED964B69AF4884E9312F0759","msg_type":"execute_request","session":"15674E9EC9184A3882F65426EEAD752E"}', '{}', '{"execution_state":"busy"}']
to_send: ['kernel.43636566-0f3c-42f2-88af-99991797eac0.pyin', '<IDS|MSG>', '065205691e7612bae8c22198d6d62917', '{"username":"kernel","msg_type":"pyin","msg_id":"e2dc0bfe-f69c-47f6-a93b-5a8da7e1d426","version":[0,14,0,"dev"],"session":"c0bf1dc7-0d1d-4c9d-8cdf-cbc004f22d0a","date":"2012-12-04T00:17:29.265194"}', '{"username":"username","msg_id":"139ED717ED964B69AF4884E9312F0759","msg_type":"execute_request","session":"15674E9EC9184A3882F65426EEAD752E"}', '{}', '{"execution_count":1,"code":"def f(a): \\n    result = a + 1\\n    return result\\n"}']
 */

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
    for (std::list<std::string>::const_reverse_iterator rit = idents.rbegin();
         rit != idents.rend();
         rit++) {
        messages->push_front(convert(*rit));
    }
}


std::list<zmq::message_t*>::const_iterator _begin_msg(const std::list<zmq::message_t*> & messages, std::list<std::string>* idents)
{

    for (std::list<zmq::message_t*>::const_iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        const zmq::message_t * m = *it;
        if (!m) {
            LOG(WARNING) << "ignoring NULL message";
            continue;
        }

        if (m->size() == 9 && memcmp((void*)IPythonMessage::DELIM, m->data(), 9) == 0) {
            // return the next element
            return ++it;
        }
        if (idents) {
            std::string id((const char*)m->data(), m->size());
            idents->push_back(id);
        }
    }
    return messages.end();

}

// to_send: ['<IDS|MSG>', 'b3f7486530469f95104f5d08b671ab49', '{"username":"jakob","msg_type":"execute_request","msg_id":"cbf262c7-81ff-4c0d-824f-8a6ccdb6d35b","version":[0,14,0,"dev"],"session":"8031b811-0d72-4a5f-96ab-83867665cc96","date":"2012-12-03T22:43:28.182231"}', '{}', '{}', '{"user_variables":[],"code":"1\\n","silent":false,"allow_stdin":true,"store_history":true,"user_expressions":{}}']
bool IPythonMessage::deserialize(const std::list<zmq::message_t*> & messages) {

    idents.clear();
    std::list<zmq::message_t*>::const_iterator msg = _begin_msg(messages, &idents);
    if (msg == messages.end()) {
        LOG(WARNING) << "Failed to separate idents and msg_list";
        return false;
    }
    DLOG(INFO) << "Separated message list from rest ";
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

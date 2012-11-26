
#include "ipython_message.h"
#include <sstream>


// IPythonMessage::IPythonMessage() {
// }

const char * IPythonMessage::DELIM = "<IDS|MSG>";

zmq::message_t * convert(std::string s) {
    char *buf = new char[s.size()];
    memcpy(buf, s.data(), s.size());
    return new zmq::message_t((void*)buf, s.size(), NULL);
}

bool IPythonMessage::serialize(std::list<zmq::message_t*> *messages) const {

    messages->push_back(convert(session_id));
    messages->push_back(convert(DELIM));
    messages->push_back(convert(hmac));
    {
        std::ostringstream os;
        header.stringify(os);
        messages->push_back(convert(os.str()));
    }
    {
        std::ostringstream os;
        parent.stringify(os);
        messages->push_back(convert(os.str()));
    }
    {
        std::ostringstream os;
        metadata.stringify(os);
        messages->push_back(convert(os.str()));
    }
    {
        std::ostringstream os;
        content.stringify(os);
        messages->push_back(convert(os.str()));
    }
}

bool IPythonMessage::deserialize(const std::list<zmq::message_t*> & messages) {

    std::list<zmq::message_t*>::const_iterator msg = messages.begin();
    if (msg == messages.end()) return false;

    session_id = std::string((char*)(*msg)->data(), (*msg)->size());

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

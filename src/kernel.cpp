
#include "kernel.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <map>
#include <uuid/uuid.h>

using zmq::socket_t;


std::string _get_zmq_last_endpoint(socket_t & socket) {
    char buf[512];
    size_t buf_size = sizeof(buf);
    //int retval =
    socket.getsockopt(ZMQ_LAST_ENDPOINT, (void*)buf, &buf_size);
    // assert(retval == 0);
    // buf_size is modified.
    return std::string(buf, buf_size);
}

bool _try_zmq_endpoint_get_port(const std::string endpoint, uint16_t & port) {
    if (endpoint.substr(0, 3) != "tcp") {
        return false;
    }

    size_t pos = endpoint.rfind(':');
    if (pos == std::string::npos) {
        return false;
    }

    std::string sport = endpoint.substr(pos + 1);
    port = strtol(sport.c_str(), NULL, 10);
    return errno != EINVAL;
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


std::ostream & _json_stringify(const std::map<std::string, std::string> & obj,
                               std::ostream & os)
{
    os << "{" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = obj.begin();
         it != obj.end();
         ++it) {
        if (it != obj.begin()) {
            os << "," << std::endl;
        }
        os << "\t\"" << it->first << "\": " << it->second ;
    }
    os << std::endl << "}" << std::endl;

    return os;
}

std::ostream & Kernel::TCPInfo::json_stringify(std::ostream & os) const {
    std::map<std::string, std::string> atts;
    atts.insert(std::make_pair("transport", "\"" + transport + "\""));
    atts.insert(std::make_pair("ip", "\"" + ip + "\""));
    atts.insert(std::make_pair("key", "\"" + key + "\""));
    std::ostringstream oss;
    oss << stdin_port;
    atts.insert(std::make_pair("stdin_port", oss.str()));
    oss.str(""); oss << hb_port ;
    atts.insert(std::make_pair("hb_port", oss.str()));
    oss.str(""); oss << iopub_port ;
    atts.insert(std::make_pair("iopub_port", oss.str()));
    oss.str(""); oss << shell_port ;
    atts.insert(std::make_pair("shell_port", oss.str()));

    return _json_stringify(atts, os);
}


/*
 ident1,ident2,...,DELIM,p_header,p_parent,p_metadata,p_content,buffer1,buffer2,..]
 */


struct Message {
    uuid_t session_id;
    std::string hmac;
    std::string header_json;
    std::string parent_json;
    std::string metadata_json;
    std::string content_json;
};


struct SessionMessageParser {

    SessionMessageParser();

    enum ParseState {
        SESSION_ID = 1,
        DELIM = 2,
        HMAC = 3,
        HEADER = 4,
        PARENT = 5,
        METADATA = 6,
        CONTENT = 7,
        FINISHED = 8
    };


    bool is_complete();
    bool parse(const char * data, size_t length);
    void reset();

    int state;
    Message message;
};

SessionMessageParser::SessionMessageParser()
    :  state(SESSION_ID)
{
}

bool SessionMessageParser::is_complete() {
    return state == FINISHED;
}

bool SessionMessageParser::parse(const char * data, size_t size) {
    if (state == SESSION_ID) {
        if (size != 36) return false;
        std::string s(data, size);
        std::istringstream is(s);
        _uuid_parse(is, message.session_id);
        state++;
    }
    else if (state == DELIM){
        if (size != 9) return false;
        if (memcmp((void*)"<IDS|MSG>", (void*)data, 9) != 0 ) return false;
        state++;
    }
    else if (state == HMAC) {
        message.hmac = std::string(data, size);
        state++;
    }
    else if (state == HEADER ) {
        message.header_json = std::string(data, size);
        state++;
    }
    else if (state == PARENT) {
        message.parent_json = std::string(data, size);
        state++;
    }
    else if (state == METADATA) {
        message.metadata_json = std::string(data, size);
        state++;
    }
    else if (state == CONTENT) {
        message.content_json = std::string(data, size);
        state++;
    }
    else {
        return false;
    }
    return true;
}



Kernel::Kernel(zmq::context_t &ctx, const uuid_t & kernelid, const std::string &ip)
    : _ctx(ctx),
      _hb(ctx, ZMQ_REP),
      _stdin(ctx, ZMQ_ROUTER),
      _iopub(ctx, ZMQ_PUB),
      _shell(ctx, ZMQ_ROUTER),
      _ip(ip),
      _shutdown(false)
{
    uuid_copy(_kernelid, kernelid);
    _tcp_info.ip = _ip;
    std::ostringstream uuidstr;
    _uuid_stringify(_kernelid, uuidstr);
    _kernelid_string = uuidstr.str();
    uuid_generate(_sessionid);
    uuidstr.str("");
    _uuid_stringify(_sessionid, uuidstr);
    _sessionid_string = uuidstr.str();
    _tcp_info.key = _sessionid_string;
}

void Kernel::start() {
   // use ephemeral port
    std::string ep = "tcp://" + _ip + ":*";
   _hb.bind(ep.c_str());
   _stdin.bind(ep.c_str());
   _iopub.bind(ep.c_str());
   _shell.bind(ep.c_str());

   const std::string hb_endpoint = _get_zmq_last_endpoint(_hb);
   const std::string stdin_endpoint = _get_zmq_last_endpoint(_stdin);
   const std::string iopub_endpoint = _get_zmq_last_endpoint(_iopub);
   const std::string shell_endpoint = _get_zmq_last_endpoint(_shell);

   // std::cout << "stdin endpoint: " << stdin_endpoint << std::endl;
   uint16_t port = 0;
   _try_zmq_endpoint_get_port(hb_endpoint, _tcp_info.hb_port);
   _try_zmq_endpoint_get_port(stdin_endpoint, _tcp_info.stdin_port);
   _try_zmq_endpoint_get_port(iopub_endpoint, _tcp_info.iopub_port);
   _try_zmq_endpoint_get_port(shell_endpoint, _tcp_info.shell_port);

}


void Kernel::message_loop() {

    SessionMessageParser message_parser;
    do  {
        zmq::pollitem_t items [1];
        /* First item refers to ØMQ socket 'socket' */
        items[0].socket = _shell;
        items[0].events = ZMQ_POLLIN;

        int timeout = -1;
        int rc = zmq::poll(items, 1, timeout );
        if (rc >= 0) { //success
            // number of events
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t msg;
                _shell.recv(&msg);
                if (!message_parser.parse((char*)msg.data(), msg.size())) {
                    std::cout << "failed to parse!" << std::endl;
                    std::cout << "received " << msg.size() << " bytes" << std::endl;
                    std::cout << "data:  " << std::string((char*)msg.data(), msg.size()) << std::endl;
                }
                else if (message_parser.is_complete()) {
                    std::cout << "completed parsing!" << std::endl;
                    std::cout << "now dispatch." << std::endl;
                    std::cout << "content: " << message_parser.message.content_json << std::endl;
                    message_parser = SessionMessageParser();
                }
            }
        }
    } while (!_shutdown);
}


const Kernel::TCPInfo & Kernel::endpoint_info() const {
    return _tcp_info;
}

const std::string & Kernel::id() const  {
    return _kernelid_string;
}
const std::string & Kernel::sessionid() const  {
    return _sessionid_string;
}



int main(int argc, char** argv) {
    int io_threads = 1;
    std::string ip = "127.0.0.1";
    zmq::context_t ctx(io_threads);

    std::string id;
    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (::strcmp("--id", arg) == 0) {
            id = argv[++i];
            std::cout << "reusing id " << id << std::endl;
       }
    }

    uuid_t kernelid;
    if (id.empty()) {
        uuid_generate(kernelid);
    }
    else {
        std::istringstream is(id);
        _uuid_parse(is, kernelid);
        std::ostringstream os;
        _uuid_stringify(kernelid, os);
        assert( os.str() == id );
    }

    Kernel kernel(ctx, kernelid, ip);

    try {
        kernel.start();
        // kernel.write_file();
    } catch (const std::exception & e) {
        std::cerr << "error " << e.what() << std::endl;
    }

    std::ostringstream connectionfile;
    connectionfile << "kernel-" << kernel.id() << ".json";

    {
        std::ofstream fs(connectionfile.str().c_str());
        kernel.endpoint_info().json_stringify(fs);
    }
    std::cout << "wrote " << connectionfile.str() << std::endl;

    // std::cout << "started " << kernel.id() << std::endl;
    // std::cout << "started " << kernel.sessionid() << std::endl;

    kernel.message_loop();
}

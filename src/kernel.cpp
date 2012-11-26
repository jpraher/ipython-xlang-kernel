
#include "kernel.h"
#include "ipython_message.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <map>
#include <uuid/uuid.h>

using zmq::socket_t;


bool sockopt_rcvmore(zmq::socket_t & socket)
{
    int64_t rcvmore;
    size_t type_size = sizeof(int64_t);
    socket.getsockopt(ZMQ_RCVMORE, &rcvmore, &type_size);
    return rcvmore ? true : false;
}


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

Kernel::TCPInfo::TCPInfo() {
}

Kernel::TCPInfo::TCPInfo(const json::object_value & object)
    : _data(object)
{
}


const std::string &Kernel::TCPInfo::transport() const
{
    static const std::string EMPTY = "";
    const std::string * v = _data.string("transport");
    if (v) {
        return *v;
    }
    return EMPTY;
}
void Kernel::TCPInfo::set_transport(const std::string & transport) {
    _data.set_string("transport",transport);
}

const std::string & Kernel::TCPInfo::ip() const {
    static const std::string EMPTY = "";
    const std::string * v = _data.string("ip");
    if (v) {
        return *v;
    }
    return EMPTY;
}
void Kernel::TCPInfo::set_ip(const std::string & ip) {
    _data.set_string("ip", ip);
}

uint16_t Kernel::TCPInfo::stdin_port() const {
    const int64_t* v = _data.int64("stdin_port");
    if (v) {
        return *v;
    }
    return 0;
}

void Kernel::TCPInfo::set_stdin_port(uint16_t port) {
    _data.set_int64("stdin_port", port);
}

uint16_t    Kernel::TCPInfo::hb_port() const {
    const int64_t* v = _data.int64("stdin_port");
    if (v) {
        return *v;
    }
    return 0;
}
void Kernel::TCPInfo::set_hb_port(uint16_t port) {
    _data.set_int64("hb_port", port);
}

uint16_t Kernel::TCPInfo::shell_port() const {
    const int64_t* v = _data.int64("shell_port");
    if (v) {
        return *v;
    }
    return 0;
}
void Kernel::TCPInfo::set_shell_port(uint16_t port) {
    _data.set_int64("shell_port", port);
}

uint16_t Kernel::TCPInfo::iopub_port() const {
    const int64_t* v = _data.int64("iopub_port");
    if (v) {
        return *v;
    }
    return 0;
}
void Kernel::TCPInfo::set_iopub_port(uint16_t port) {
    _data.set_int64("iopub_port", port);
}

const std::string & Kernel::TCPInfo::key() const {
    static const std::string EMPTY = "";
    const std::string * v = _data.string("ip");
    if (v) {
        return *v;
    }
    return EMPTY;

}
void Kernel::TCPInfo::set_key(const std::string & key)
{
    _data.set_string("key", key);
}

const json::object_value & Kernel::TCPInfo::json() const
{
    return _data;
}


/*
 ident1,ident2,...,DELIM,p_header,p_parent,p_metadata,p_content,buffer1,buffer2,..]
 */

/*


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

*/

Kernel::Kernel(zmq::context_t &ctx, const uuid_t & kernelid, const std::string &ip, MsgCallback * shell_handler)
    : _ctx(ctx),
      _hb(ctx, ZMQ_REP),
      _stdin(ctx, ZMQ_ROUTER),
      _iopub(ctx, ZMQ_PUB),
      _shell(ctx, ZMQ_ROUTER),
      _shell_handler(shell_handler),
      _ip(ip),
      _shutdown(false)
{
    uuid_copy(_kernelid, kernelid);
    _tcp_info.set_ip(_ip);
    std::ostringstream uuidstr;
    _uuid_stringify(_kernelid, uuidstr);
    _kernelid_string = uuidstr.str();
    uuid_generate(_sessionid);
    uuidstr.str("");
    _uuid_stringify(_sessionid, uuidstr);
    _sessionid_string = uuidstr.str();
    _tcp_info.set_key(_sessionid_string);
}

void Kernel::start() {
   // use ephemeral port
    std::string ep = "tcp://" + _ip + ":*";
   _hb.bind(ep.c_str());
   _stdin.bind(ep.c_str());
   _iopub.bind(ep.c_str());
   _shell.bind(ep.c_str());

   const std::string hb_endpoint    = _get_zmq_last_endpoint(_hb);
   const std::string stdin_endpoint = _get_zmq_last_endpoint(_stdin);
   const std::string iopub_endpoint = _get_zmq_last_endpoint(_iopub);
   const std::string shell_endpoint = _get_zmq_last_endpoint(_shell);

   // std::cout << "stdin endpoint: " << stdin_endpoint << std::endl;
   uint16_t port = 0;
   _try_zmq_endpoint_get_port(hb_endpoint, port);
   _tcp_info.set_hb_port(port);
   port = 0;
   _try_zmq_endpoint_get_port(stdin_endpoint, port);
   _tcp_info.set_stdin_port(port);
   port = 0;
   _try_zmq_endpoint_get_port(iopub_endpoint, port);
   _tcp_info.set_iopub_port(port);
   port = 0;
   _try_zmq_endpoint_get_port(shell_endpoint, port);
   _tcp_info.set_shell_port(port);

}


void _receive(zmq::socket_t & socket, std::list<zmq::message_t*> *result) {
    do {
        zmq::message_t * msg = new zmq::message_t();
        // TODO: error handling
        socket.recv(msg);
        result->push_back(msg);
    } while (sockopt_rcvmore(socket));
}

void _send(zmq::socket_t & socket, std::list<zmq::message_t*> &msg_list) {
    std::list<zmq::message_t*>::const_iterator last = --msg_list.end();
    for (std::list<zmq::message_t*>::const_iterator it = msg_list.begin();
         it != msg_list.end();
         ++it) {
        int flags = (it == last) ? 0 : ZMQ_SNDMORE;
        // TODO: error handling
        socket.send(*it, flags);
    }
}


void Kernel::message_loop() {

    //SessionMessageParser message_parser;
    do  {
        zmq::pollitem_t items [1];
        /* First item refers to ØMQ socket 'socket' */
        items[0].socket = _shell;
        items[0].events = ZMQ_POLLIN;

        int timeout = -1;
        int rc = zmq::poll(items, 1, timeout );
        if (rc >= 0) { //success
            // TODO generalize on all sockets
            // number of events
            if (items[0].revents & ZMQ_POLLIN) {
                std::list<zmq::message_t*> request;
                std::list<zmq::message_t*> response;
                _receive(_shell,&request);
                _shell_handler->handle(request, &response);
                _send(_shell, response);
                /*
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
                */
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


class PrintOutCallback : public MsgCallback{
public:
    virtual ~PrintOutCallback();
    virtual void handle(const std::list<zmq::message_t*> &request_msg_list,
                        std::list<zmq::message_t*> * response_msg_list);
};


PrintOutCallback::~PrintOutCallback() {}
void PrintOutCallback::handle(const std::list<zmq::message_t*> &request_msg_list,
                              std::list<zmq::message_t*> * response_msg_list) {

    IPythonMessage request;
    request.deserialize(request_msg_list);
    /*

     */
    request.content.stringify(std::cout) << std::endl;

    IPythonMessage response;

    response.session_id = request.session_id;
    response.hmac = request.hmac;

    response.serialize(response_msg_list);
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

    PrintOutCallback shellCallback;
    Kernel kernel(ctx, kernelid, ip, &shellCallback);

    try {
        kernel.start();
    } catch (const std::exception & e) {
        std::cerr << "error " << e.what() << std::endl;
    }

    std::ostringstream connectionfile;
    connectionfile << "kernel-" << kernel.id() << ".json";
    {
        std::ofstream fs(connectionfile.str().c_str());
        kernel.endpoint_info().json().stringify(fs);
    }
    std::cout << "wrote " << connectionfile.str() << std::endl;

    kernel.message_loop();
}

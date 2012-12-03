/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "kernel.h"
#include "ipython_message.h"
#include "hmac.h"
#include "tthread/tinythread.h"
#include "delegate.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <map>
#include <uuid/uuid.h>
#include <glog/logging.h>
#include <time.h>


using zmq::socket_t;
using tthread::thread;

bool sockopt_rcvmore(zmq::socket_t & socket)
{
    int64_t rcvmore = 0;
    size_t type_size = sizeof(int64_t);
    socket.getsockopt(ZMQ_RCVMORE, &rcvmore, &type_size);
    // std::cout << rcvmore << std::endl;
    return rcvmore ? true : false;
}


void _send(zmq::socket_t & socket, std::list<zmq::message_t*> &msg_list) {
    std::list<zmq::message_t*>::const_iterator last = --msg_list.end();
    int i = 0;
    for (std::list<zmq::message_t*>::const_iterator it = msg_list.begin();
         it != msg_list.end();
         ++it) {
        int flags = (it == last) ? 0 : ZMQ_SNDMORE;
        DLOG(INFO) << "sending msg " << i++ << " flags " << flags;
        socket.send(*(*it), flags);
    }
}

SocketChannel::SocketChannel(zmq::socket_t & sock, const std::string &key)
    : _socket(&sock),
      _key(key)
{
    DLOG(INFO) << "socket key " << key;
}

void SocketChannel::send(const Message & message) {
    assert(_socket);

    std::list<zmq::message_t*> msg_list;
    message.serialize(_key, &msg_list);
    _send(*_socket, msg_list);
    // free messages
    for (std::list<zmq::message_t*>::iterator it = msg_list.begin();
         it != msg_list.end();
         ++it) {
        if (*it) delete *it;
        *it = NULL;
    }
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

Kernel::TCPInfo::TCPInfo(const TCPInfo & object)
    : _data(object._data)
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
    const int64_t* v = _data.int64("hb_port");
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
    const std::string * v = _data.string("key");
    if (v) {
        return *v;
    }
    return EMPTY;

}
void Kernel::TCPInfo::set_key(const std::string & key)
{
    _data.set_string("key", key);
}

json::object_value* Kernel::TCPInfo::mutable_json() {
    return &_data;
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

Kernel::Kernel(zmq::context_t &ctx, const Kernel::TCPInfo &info, ExecuteHandler * shell_handler)
    : _ctx(ctx),
      _tcp_info(info),
      _hb(ctx, ZMQ_REP),
      _stdin(ctx, ZMQ_ROUTER),
      _iopub(ctx, ZMQ_PUB),
      _shell(ctx, ZMQ_ROUTER),
      _hmackey_string(info.key()),
      _stdinChannel(_stdin, info.key()),
      _iopubChannel(_iopub, info.key()),
      _shellChannel(_shell, info.key()),
      _shell_handler(shell_handler),
      _ip(info.ip()),
      _shutdown(false),
      _hb_count(0),
      _hb_thread(NULL),
      _run_hb_delegate(NULL),
      _exec_ctx(_iopubChannel, _shellChannel),
      _stdout_redirector(STDOUT_FILENO, delegate1_t<EContext, void, const std::string&>(&_exec_ctx, &EContext::handle_stdout)),
      _stderr_redirector(STDERR_FILENO, delegate1_t<EContext, void, const std::string&>(&_exec_ctx, &EContext::handle_stderr))
{
}

uint16_t _bind_tcp(zmq::socket_t & sock, const std::string & ip, uint16_t port) {
    std::ostringstream ep;
    ep <<  "tcp://" << ip <<  ":";
    if (port == 0) {
        ep << "*";
    } else {
        ep << port;
    }
    DLOG(INFO) << "ZMQ binding endpoint " << ep.str();

    sock.bind(ep.str().c_str());
    if (port == 0) {
        const std::string endpoint = _get_zmq_last_endpoint(sock);
        uint16_t out_port = 0;
        _try_zmq_endpoint_get_port(endpoint, out_port);
        return out_port;
    }
    return port;
}

void Kernel::start() {
    _bind_tcp(_hb, _ip, _tcp_info.hb_port());
    _bind_tcp(_stdin, _ip, _tcp_info.stdin_port());
    _bind_tcp(_iopub, _ip, _tcp_info.iopub_port());
    _bind_tcp(_shell, _ip, _tcp_info.shell_port());

    delegate_t<Kernel> * _run_hb_delegate = new delegate_t<Kernel>(this, &Kernel::run_heartbeat);
    _hb_thread = new thread(_run_hb_delegate->dispatch, _run_hb_delegate);

    // _stdout_redirector.start();
    // _stderr_redirector.start();
}


void _receive(zmq::socket_t & socket, std::list<zmq::message_t*> *result) {
    do {
        zmq::message_t * msg = new zmq::message_t();
        // TODO: error handling
        socket.recv(msg);
        result->push_back(msg);
    } while (sockopt_rcvmore(socket));
}


void Kernel::run_heartbeat() {
    DLOG(INFO) << "entering heartbeat run loop.";
    zmq_device(ZMQ_FORWARDER, _hb, _hb);
    DLOG(INFO) << "leaving heartbeat run loop.";
}

void Kernel::message_loop() {
    DLOG(INFO) << "entering message loop.";
    do  {
        zmq::pollitem_t items [1];
        /* First item refers to ØMQ socket 'socket' */
        items[0].socket = _shell;
        items[0].events = ZMQ_POLLIN;
        // items[1].socket = _hb;
        // items[1].events = ZMQ_POLLIN;

        int timeout = -1;
        int rc = zmq::poll(items, 1, timeout );
        if (rc >= 0) { //success
            // TODO generalize on all sockets
            // number of events
            if (items[0].revents & ZMQ_POLLIN) {
                std::list<zmq::message_t*> msg_list;
                // std::list<zmq::message_t*> response;
                DLOG(INFO) << "receiving";
                _receive(_shell,&msg_list);
                DLOG(INFO) << "received " << msg_list.size() << std::endl;
                Message * request = _shell_handler->create_request();
                assert(request != NULL);
                request->deserialize(msg_list);
                Message::free(msg_list);
                _shell_handler->execute(_exec_ctx, request);
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
const std::string & Kernel::key() const  {
    return _hmackey_string;
}


class PrintOutCallback : public ExecuteHandler{
public:
    PrintOutCallback();
    virtual ~PrintOutCallback();

    virtual Message * create_request();
    virtual void execute(EContext & ctx,
                         Message *request);
private:
    size_t _execution_count;
};

PrintOutCallback::PrintOutCallback()
    : _execution_count(0)
{
}

PrintOutCallback::~PrintOutCallback() {}

Message * PrintOutCallback::create_request() {
    return new IPythonMessage();
}
void PrintOutCallback::execute(EContext & ctx,
                               Message *requestMsg)
{
    DLOG(INFO) << "PrintOutCallback::handle()";

    IPythonMessage * request = dynamic_cast<IPythonMessage*>(requestMsg);
    assert(request != NULL);

    // get info if we are silent
    bool silent = json::get(request->content.boolean("silent"), false);

    const std::string *codePtr = request->content.string("code");
    std::string code;
    if (codePtr) {
        code = *codePtr;
    }
    // trivial RTrim
    while (code.size() > 0 && isspace(code[code.size() - 1])) {
        code = code.substr(0, code.size() - 1);
    }

    /*
     */
    std::cout << "REQUEST header>" <<  request->header.to_str() << std::endl;
    std::cout << "REQUEST content>" <<  request->content.to_str() << std::endl;
    std::cout << "REQUEST metadata>" <<  request->metadata.to_str() << std::endl;

    if (!silent) {
        IPythonMessage pyin;
        pyin.session_id = request->session_id;
        pyin.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyin.header.set_string("msg_id",oss.str());
        }
        pyin.header.set_string("msg_type", "pyin");
        pyin.header.set_string("session", request->session_id);
        pyin.content.set_int64("execution_count", _execution_count);

        json::object_value * pyout_data = pyin.content.mutable_object("code");
        pyin.content.set_string("code", json::get(request->content.string("code"), json::value::EMPTY_STRING));
        std::cout << "PYIN content >" << pyin.content.to_str() << std::endl;
        ctx.io().send(pyin);
    }



    IPythonMessage response;

    response.session_id = request->session_id;
    response.content.set_string("status", "ok");
    response.content.set_int64("execution_count", ++_execution_count);

    // ok case
    response.content.mutable_array("payload");
    response.content.mutable_object("user_variables");
    response.content.mutable_object("user_expressions");

    // get millis since
    response.metadata.set_boolean("dependencies_met", true);
    // TODO set ident
    response.metadata.set_string("engine", "ident");

    time_t msnow = time((time_t*)NULL) * 1000;
    response.metadata.set_int64("started",  msnow);
    response.metadata.merge(request->metadata);

    // always new msgid ...

    response.metadata.set_string("status", "ok");
    // {"date":"2012-11-27T21:55:17.543998","msg_id":"a9522f9c-f097-4921-9d28-35688bd7e32a","msg_type":"execute_request","session":"2efc817e-52d4-4945-aa36-93b755f238fb","username":"jakob","version":[0,14,0,"dev"]}

    {
        uuid_t msg_id;
        uuid_generate(msg_id);
        std::ostringstream oss;
        _uuid_stringify(msg_id, oss);
        response.header.set_string("msg_id",oss.str());
    }
    if (request->header.string("username")) {
        response.header.set_string("username", *request->header.string("username"));
    }
    //if (request.header.string("msg_type")) {
    response.header.set_string("msg_type", "execute_reply");
    // *request.header.string("msg_type"));
    //}
    if (request->header.string("session")) {
        response.header.set_string("session", *request->header.string("session"));
    }

    response.parent.merge(request->header);
    std::cout << "RESPONSE header  >" << response.header.to_str() << std::endl;
    std::cout << "RESPONSE content >" << response.content.to_str() << std::endl;
    std::cout << "RESPONSE metadata>" << response.metadata.to_str() << std::endl;

    ctx.shell().send(response);

    if (!silent && !code.empty()) {
        IPythonMessage pyout;
        pyout.session_id = request->session_id;
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "pyout");
        pyout.header.set_string("session", request->session_id);
        pyout.content.set_int64("execution_count", _execution_count);

        json::object_value * pyout_data = pyout.content.mutable_object("data");
        pyout_data->set_string("text/plain", code);
        std::cout << "PYOUT content >" << pyout.content.to_str() << std::endl;
        ctx.io().send(pyout);
    }

    if (!silent) {
        std::cout.flush();
        fflush(stdout);
        IPythonMessage pyout;
        pyout.session_id = request->session_id;
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "stream");
        pyout.header.set_string("session", request->session_id);
        pyout.content.set_string("data", ctx.stdout());
        pyout.content.set_string("name", "stdout");
        ctx.io().send(pyout);
    }
    if (!silent) {
        std::cerr.flush();
        fflush(stderr);
        IPythonMessage pyout;
        pyout.session_id = request->session_id;
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "stream");
        pyout.header.set_string("session", request->session_id);
        pyout.content.set_string("data", ctx.stderr());
        pyout.content.set_string("name", "stderr");
        ctx.io().send(pyout);
    }
    // free request.
}



int main(int argc, char** argv) {

    google::InitGoogleLogging(argv[0]);

    /*
starting kernel {'extra_arguments': [u"--KernelApp.parent_appname='ipython-notebook'"], 'cwd': u'/Users/jakob/extsrc/ipython'}
Starting new kernel af0f4df7-3741-4ec5-9d6a-74c1142dcff6
/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-3741-4ec5-9d6a-74c1142dcff6.json
ipkernel launch_kernel: args (), kwargs {'extra_arguments': [u"--KernelApp.parent_appname='ipython-notebook'"], 'cwd': u'/Users/jakob/extsrc/ipython', 'fname': u'/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-3741-4ec5-9d6a-74c1142dcff6.json'}
entry_point - launching kernel ['/usr/bin/python', '-c', 'from IPython.zmq.ipkernel import main; main()', '-f', u'/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-
     */

    // '-f', u'/Users/jakob/.ipython/profile_default/security/kernel-524ecbc8-3573-424c-ab97-77f9154b1c7b.json'

    int io_threads = 1;
    std::string ip = "127.0.0.1";
    zmq::context_t ctx(io_threads);

    DLOG(INFO) << "Starting kernel";
    std::string id;
    std::string existing_file;
    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (::strcmp("--id", arg) == 0 && i + 1 < argc) {
            id = argv[++i];
            std::cout << "reusing id " << id << std::endl;
        }
        else if (::strcmp("-f", arg) == 0 && i + 1 < argc) {
            existing_file = argv[++i];
            DLOG(INFO)<< "existing configuration " << existing_file ;
        }
    }

    Kernel::TCPInfo tcpInfo;
    if (!existing_file.empty()) {
        std::ifstream fs(existing_file.c_str());
        json::parser p(fs);
        bool result = p.parse(tcpInfo.mutable_json());
        if (!result) {
            // failed to parse
            LOG(ERROR) << "Failed to parse " << existing_file ;
            return 1;
        }
    } else {

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

        std::string key_string;
        std::string key_bin;
        {
            uuid_t key;
            uuid_generate(key);
            key_bin = std::string((char*)key, sizeof(key));
            std::ostringstream uuidstr;
            _uuid_stringify(key, uuidstr);
            key_string = uuidstr.str();
        }
    }

    PrintOutCallback shellCallback;
    Kernel kernel(ctx, tcpInfo,  &shellCallback);

    try {
        kernel.start();
    } catch (const std::exception & e) {
        LOG(ERROR) << "Failed to start kernel: " << e.what();
        return 2;
    }

    if (existing_file.empty()) {
        std::ostringstream connectionfile;
        connectionfile << "kernel-" << kernel.id() << ".json";
        {
            std::ofstream fs(connectionfile.str().c_str());
            kernel.endpoint_info().json().stringify(fs);
        }
        std::cout << "wrote " << connectionfile.str() << std::endl;
    }
    kernel.message_loop();

}

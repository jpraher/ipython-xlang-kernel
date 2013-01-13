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

void _receive(zmq::socket_t & socket, std::list<zmq::message_t*> *result) {
    do {
        zmq::message_t * msg = new zmq::message_t();
        // TODO: error handling
        socket.recv(msg);
        result->push_back(msg);
    } while (sockopt_rcvmore(socket));
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

/*
  let's assume that there is a
 */
bool SocketChannel::recv(Message *message, int timeout) {
    assert(_socket);
    assert(message);

    _socket->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    std::list<zmq::message_t*> msg_list;
    _receive(*_socket, &msg_list);
    bool ok = message->deserialize(msg_list);
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

Kernel::Kernel(int number_of_io_threads,
               const Kernel::TCPInfo &info)
    : _ctx(new zmq::context_t(number_of_io_threads)),
      _tcp_info(info),
      _ident(_generate_uuid()),
      _hmackey_string(info.key()),
      _shell_handler(NULL),
      _ip(info.ip()),
      _shutdown(false),
      _shutted_down(false),
      _started(false),
      _hb_count(0),
      _hb_thread(NULL),
      _run_hb_delegate(NULL),
      _stdout_redirector(STDOUT_FILENO),
      _stderr_redirector(STDERR_FILENO)
{
}

Kernel::~Kernel() {
    if (!_shutdown) {
        shutdown();
    }
    DLOG(INFO) << "Calling ~Kernel";
    google::FlushLogFiles(google::INFO);
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
    if (_started) {
        return;
    }

    DLOG(INFO) << "creating new sockets";

    _hb.reset(new zmq::socket_t(*_ctx, ZMQ_REP));
    _bind_tcp(*_hb, _ip, _tcp_info.hb_port());

    _stdin.reset(new zmq::socket_t(*_ctx, ZMQ_ROUTER));
    _bind_tcp(*_stdin, _ip, _tcp_info.stdin_port());

    _iopub.reset(new zmq::socket_t(*_ctx, ZMQ_PUB));
    _bind_tcp(*_iopub, _ip, _tcp_info.iopub_port());

    _shell.reset(new zmq::socket_t(*_ctx, ZMQ_ROUTER));
    _bind_tcp(*_shell, _ip, _tcp_info.shell_port());

    DLOG(INFO) << "bound sockets";
    _stdinChannel.reset(new SocketChannel(*_stdin,  _hmackey_string));
    _iopubChannel.reset(new SocketChannel(*_iopub,  _hmackey_string));
    _shellChannel.reset(new SocketChannel(*_shell,  _hmackey_string));
    DLOG(INFO) << "channels initialized";

    _exec_ctx.reset(new EContext(_ident,*_iopubChannel, *_shellChannel, *_stdinChannel,
                                 _stdout_redirector, _stderr_redirector));

    _shutdown = false;

    _run_hb_delegate.reset(new delegate_t<Kernel>(this, &Kernel::run_heartbeat));
    _hb_thread.reset(new thread(_run_hb_delegate->dispatch, _run_hb_delegate.get()));

    _stdout_redirector.start();
    _stderr_redirector.start();

    _msg_loop_delegate.reset(new delegate_t<Kernel>(this, &Kernel::message_loop));
    _msg_loop_thread.reset(new thread(_msg_loop_delegate->dispatch, _msg_loop_delegate.get()));

    _started = true;
    _shutted_down = false;
    DLOG(INFO) << "kernel started";
}


void Kernel::set_shell_handler(ExecuteHandler* handler) {
    _shell_handler.reset(handler);
}

void Kernel::stop() {
    DLOG(INFO) << "Kernel::stop _started=" <<  _started << ", _shutdown=" << _shutdown ;
    google::FlushLogFiles(google::INFO);
    if (!_started) return;
    if (_shutdown) return;
    _shutdown = true;

    // _stderr_redirector.stop();
    _stdout_redirector.stop();
    if (_msg_loop_thread.get()) {
        // can be invoked from inside the message_loop
        // self join is nop-operation
        _msg_loop_thread->join();
        // _msg_loop_thread.reset(NULL);
        // _msg_loop_delegate.reset(NULL);
    }

    DLOG(INFO) << "After join _msg_loop_thread";
    if (_hb_thread.get()) {
        // _hb->close();
        _hb_thread->join();
        // _hb_thread.reset(NULL);
        // _run_hb_delegate.reset(NULL);
    }
    DLOG(INFO) << "After join _hb_thread";

    _exec_ctx.reset(NULL);
    _stdinChannel.reset(NULL);
    _iopubChannel.reset(NULL);
    _shellChannel.reset(NULL);
    _hb.reset(NULL);
    _iopub.reset(NULL);
    _stdin.reset(NULL);
    _shell.reset(NULL);
    _started = false;
}

void Kernel::shutdown() {
    this->stop();
    _shutted_down = true;
    DLOG(INFO) << "Setting shutted_down true";
}

bool Kernel::has_shutdown() {
    return _shutted_down;
}





void Kernel::run_heartbeat() {
    DLOG(INFO) << "entering heartbeat run loop.";
    do  {
        try {
            zmq::pollitem_t items [1];
            /* First item refers to ØMQ socket 'socket' */
            items[0].socket = (*_hb);
            items[0].events = ZMQ_POLLIN;
            int timeout = 1; /*microseconds*/
            int rc = zmq::poll(items, 1, timeout);
            if (rc >= 0) { //success
                // TODO generalize on all sockets
                // number of events
                if (items[0].revents & ZMQ_POLLIN) {
                    do {
                        zmq::message_t msg;
                        // TODO: error handling
                        _hb->recv(&msg);
                        int flags = sockopt_rcvmore(*_hb) ? ZMQ_SNDMORE : 0;
                        _hb->send(msg, flags);
                        //result->push_back(msg);
                    } while (sockopt_rcvmore(*_hb));
                }
            }
        } catch (const std::exception &e) {
            // Classification of exceptions that can be ignored
            DLOG(INFO) << "caught exception " << e.what();
            google::FlushLogFiles(google::INFO);
        }
    } while(!_shutdown);
    DLOG(INFO) << "leaving heartbeat run loop. ";
}

void Kernel::message_loop() {
    DLOG(INFO) << "entering message loop.";
    do  {
        zmq::pollitem_t items [1];
        /* First item refers to ØMQ socket 'socket' */
        items[0].socket = (*_shell);
        items[0].events = ZMQ_POLLIN;
        // items[1].socket = _hb;
        // items[1].events = ZMQ_POLLIN;

        // timeout is in microseconds
        int timeout = 1; /*microseconds*/
        int rc = zmq::poll(items, 1, timeout );
        if (_shutdown) break;
        if (rc >= 0) { //success
            // TODO generalize on all sockets
            // number of events
            if (items[0].revents & ZMQ_POLLIN) {
                std::list<zmq::message_t*> msg_list;
                // std::list<zmq::message_t*> response;
                // DLOG(INFO) << "receiving";
                _receive(*_shell,&msg_list);
                // DLOG(INFO) << "received " << msg_list.size() << std::endl;
                Message * request = _shell_handler->create_request();
                assert(request != NULL);
                request->deserialize(msg_list);
                Message::free(msg_list);
                try {
                    _shell_handler->execute(*_exec_ctx, request);
                }
                catch (const ShutdownRequestedException & srde) {
                    // DLOG(INFO) << "Kernel stopped" ;
                    _msg_loop_thread->detach();
                    this->shutdown();
                    DLOG(INFO) << "Kernel shutdown" ;
                    google::FlushLogFiles(google::INFO);
                    return;
                }
                catch (const std::exception & e) {
                    /* */
                    LOG(ERROR) << "failed to execute message ";
                }
            }
        }
    } while (!_shutdown);
    DLOG(INFO) << "leaving message loop.";
}


const Kernel::TCPInfo & Kernel::endpoint_info() const {
    return _tcp_info;
}

const std::string & Kernel::ident() const  {
    return _ident;
}
const std::string & Kernel::key() const  {
    return _hmackey_string;
}

EContext * Kernel::execution_context() {
    return _exec_ctx.get();
}

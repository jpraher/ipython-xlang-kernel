/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __kernel__h__
#define __kernel__h__

#include <zmq.hpp>
#include <stdint.h>
#include <string>
#include <ostream>
#include "tthread/tinythread.h"
#include "json.h"
#include "message.h"
#include "delegate.h"
#include "ioredir.h"
#include "scoped_ptr.h"


class SocketChannel : public Channel {
 public:
    SocketChannel(zmq::socket_t & socket, const std::string & key);
    virtual void send(const Message & message);
    virtual bool recv(Message * message, int timeout = -1);
 private:
    zmq::socket_t * _socket;
    std::string _key;
};

class Kernel {

public:
    struct TCPInfo {

        TCPInfo();
        TCPInfo(const TCPInfo& tcpInfo);
        TCPInfo(const json::object_value & object);

        const std::string &transport() const;
        void set_transport(const std::string & transport);

        const std::string & ip() const;
        void set_ip(const std::string & ip);

        uint16_t stdin_port() const;
        void set_stdin_port(uint16_t port);

        uint16_t    hb_port() const;
        void    set_hb_port(uint16_t port);

        uint16_t shell_port() const;
        void set_shell_port(uint16_t port);

        uint16_t iopub_port() const;
        void set_iopub_port(uint16_t port);

        const std::string & key() const;
        void set_key(const std::string & key);

        json::object_value * mutable_json() ;
        const json::object_value & json() const;
    private:
        json::object_value _data;
    };

public:
    Kernel(int number_of_io_threads,
           const TCPInfo & connection_info)
        ;

    ~Kernel();

    void start();
    void stop();

    void shutdown();

    void message_loop();
    void run_heartbeat();

    void set_shell_handler(ExecuteHandler * shellHandler);

    const std::string & ident() const;
    const std::string & key() const;

    const TCPInfo & endpoint_info() const ;

    bool has_shutdown();

    EContext * execution_context();

private:
    scoped_ptr<zmq::context_t> _ctx;     // seperate context
    TCPInfo _tcp_info;

    std::string _ident;
    std::string _ip;
    scoped_ptr<zmq::socket_t> _hb;

    scoped_ptr<zmq::socket_t> _stdin;
    scoped_ptr<zmq::socket_t> _iopub;
    scoped_ptr<zmq::socket_t> _shell;
    scoped_ptr<SocketChannel>
        _stdinChannel,
        _iopubChannel,
        _shellChannel;

    scoped_ptr<EContext> _exec_ctx;
    scoped_ptr<ExecuteHandler> _shell_handler;


    std::string _kernelid_string;
    std::string _hmackey_string;

    volatile bool _shutdown;
    volatile bool _shutted_down;
    bool _started;
    size_t _hb_count;
    // active_method pattern?
    scoped_ptr<tthread::thread>  _hb_thread;
    scoped_ptr<tthread::thread>  _msg_loop_thread;
    scoped_ptr<delegate_t<Kernel> > _run_hb_delegate;
    scoped_ptr<delegate_t<Kernel> > _msg_loop_delegate;
    redirector _stdout_redirector;
    redirector _stderr_redirector;
};

#endif

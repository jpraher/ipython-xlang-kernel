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


class SocketChannel : public Channel {
 public:
    SocketChannel(zmq::socket_t & socket, const std::string & key);
    virtual void send(const Message & message);
 private:
    zmq::socket_t * _socket;
    std::string _key;
};

class Kernel {
public:
    Kernel( zmq::context_t &ctx,
            const uuid_t & kernelid,
            const std::string &ip,
            const std::string &key,
            ExecuteHandler * shellHandler)
        ;

    struct TCPInfo {
        TCPInfo();
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

        const json::object_value & json() const;
    private:
        json::object_value _data;
    };

    void start();
    void start2();

    void message_loop();
    void run_heartbeat();

    const std::string & id() const;
    const std::string & key() const;

    const TCPInfo & endpoint_info() const ;


private:
    zmq::context_t &_ctx;
    std::string _ip;
    zmq::socket_t _hb;

    zmq::socket_t _stdin;
    zmq::socket_t _iopub;
    zmq::socket_t _shell;
    SocketChannel _stdinChannel,
                  _iopubChannel,
                  _shellChannel;

    EContext _exec_ctx;
    ExecuteHandler *_shell_handler;
    // MsgCallback *_shell_handler;

    uuid_t _kernelid;
    uuid_t _hmackey;
    std::string _kernelid_string;
    std::string _hmackey_string;
    TCPInfo _tcp_info;
    bool _shutdown;
    size_t _hb_count;
    // active_method pattern?
    tthread::thread * _hb_thread;
    delegate_t<Kernel> *_run_hb_delegate;

    redirector _stdout_redirector;
    redirector _stderr_redirector;
};

#endif

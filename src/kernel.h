
#ifndef __kernel__h__
#define __kernel__h__

#include <zmq.hpp>
#include <stdint.h>
#include <string>
#include <ostream>
#include "json.h"

class MsgCallback {
 public:
    virtual ~MsgCallback() {}
    virtual void handle(const std::list<zmq::message_t*> & request,
                        std::list<zmq::message_t*> *response) = 0;
};


class Kernel {
public:
    Kernel( zmq::context_t &ctx,
            const uuid_t & kernelid,
            const std::string &ip,
            MsgCallback * shellHandler)
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

    void message_loop();


    const std::string & id() const;
    const std::string & sessionid() const;

    const TCPInfo & endpoint_info() const ;

private:
    zmq::context_t &_ctx;
    std::string _ip;
    zmq::socket_t _hb;

    zmq::socket_t _stdin;
    zmq::socket_t _iopub;
    zmq::socket_t _shell;
    MsgCallback *_shell_handler;

    uuid_t _kernelid;
    uuid_t _sessionid;
    std::string _kernelid_string;
    std::string _sessionid_string;
    TCPInfo _tcp_info;
    bool _shutdown;
};

#endif

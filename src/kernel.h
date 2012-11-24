
#ifndef __kernel__h__
#define __kernel__h__

#include <zmq.hpp>
#include <stdint.h>
#include <string>
#include <ostream>

// using zmq::context_t;
// using zmq::socket_t;


class Kernel {
public:
    Kernel( zmq::context_t &ctx, const std::string &ip)
        ;

    struct TCPInfo {
        TCPInfo()
        :   transport("tcp"),
            stdin_port(0),
            hb_port(0),
            shell_port(0),
            iopub_port(0)
        {
        }

        std::string transport;
        std::string ip;
        uint16_t stdin_port;
        uint16_t    hb_port;
        uint16_t shell_port;
        uint16_t iopub_port;
        std::string key;

        std::ostream & json_stringify(std::ostream & os);

    };

    void start();
    void msg();

    const TCPInfo & endpoint_info() ;

private:
    zmq::context_t &_ctx;
    std::string _ip;
    zmq::socket_t _hb;

    zmq::socket_t _stdin;
    zmq::socket_t _iopub;
    zmq::socket_t _shell;


    TCPInfo _tcp_info;
};

#endif

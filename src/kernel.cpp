
#include "kernel.h"
#include <assert.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <map>
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


std::ostream & _json_stringify(const std::map<std::string, std::string> & obj,
                               std::ostream & os)
{
    os << "{" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = obj.begin();
         it != obj.end();
         ++it) {
        os << "\t\"" << it->first << "\": " << it->second << std::endl;
    }
    os << "}" << std::endl;

    return os;
}

std::ostream & Kernel::TCPInfo::json_stringify(std::ostream & os) {
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

Kernel::Kernel(zmq::context_t &ctx, const std::string &ip)
    : _ctx(ctx),
      _hb(ctx, ZMQ_REP),
      _stdin(ctx, ZMQ_ROUTER),
      _iopub(ctx, ZMQ_PUB),
      _shell(ctx, ZMQ_ROUTER),
      _ip(ip)
{
    _tcp_info.ip = _ip;
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

   _tcp_info.json_stringify(std::cout);
}


void Kernel::msg() {

}


void test(){

}

/*


# Build dict of handlers for message types
        msg_types = [ 'execute_request', 'complete_request',
                      'object_info_request', 'history_request',
                      'connect_request', 'shutdown_request',
                      'apply_request',
                    ]
 */


/*
execute_request

content = {
    # Source code to be executed by the kernel, one or more lines.
'code' : str,

# A boolean flag which, if True, signals the kernel to execute
# this code as quietly as possible.  This means that the kernel
# will compile the code with 'exec' instead of 'single' (so
# sys.displayhook will not fire), and will *not*:
#   - broadcast exceptions on the PUB socket
#   - do any logging
#   - populate any history
#
# The default is False.
'silent' : bool,

# A list of variable names from the user's namespace to be retrieved.  What
# returns is a JSON string of the variable's repr(), not a python object.
'user_variables' : list,

# Similarly, a dict mapping names to expressions to be evaluated in the
# user's dict.
'user_expressions' : dict,

# Some frontends (e.g. the Notebook) do not support stdin requests. If
# raw_input is called from code executed from such a frontend, a
# StdinNotImplementedError will be raised.
'allow_stdin' : True,

}

 */


/*
 *
 */


/*
/Users/jakob/.ipython/profile_default/security/kernel-633e91cf-aa08-465f-afdb-5391039a63b6.json
{
  "stdin_port": 60861,
  "ip": "127.0.0.1",
  "hb_port": 60862,
  "key": "74547b55-31b9-483e-8c8a-b5191cb3f3c2",
  "shell_port": 60859,
  "transport": "tcp",
  "iopub_port": 60860
}
*/



int main(int argc, char** argv) {
    int io_threads = 1;
    std::string ip = "127.0.0.1";
    zmq::context_t ctx(io_threads);
    Kernel kernel(ctx, ip);
    try {
        kernel.start();
    } catch (const std::exception & e) {
        std::cerr << "error " << e.what() << std::endl;
    }

    std::cout << "started" << std::endl;
}

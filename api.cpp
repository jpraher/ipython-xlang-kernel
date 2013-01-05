
#include "api.h"
#include "kernel.h"
#include "json.h"
#include "ioredir.h"
#include "ipython_message.h"
#include "ipython_shell_handler.h"
#include <fstream>
#include <glog/logging.h>

struct kernel_env_t {
    void * zmq_ctx;
};


void kernel_env_init(const char * app_name) {
    google::InitGoogleLogging(app_name);
}

/*
kernel_env_t * new_kernel_env(int number_threads) {
    kernel_env_t* p = alloc_init<kernel_env_t>();
    if (!p) return p;
    p->zmq_ctx = zmq_init(number_threads);
}

void free_kernel_env(kernel_env_t* k) {
    if (k && k->zmq_ctx) {
        zmq_term(k->zmq_ctx);
        k->zmq_ctx = NULL;
    }
    free(k);
}
*/

kernel_t* new_kernel_with_connection_file(/* kernel_env_t* kenv */ int number_io_threads, const char * connection_file) {

    std::ifstream fs(connection_file);
    json::parser p(fs);
    Kernel::TCPInfo tcpInfo;
    bool result = p.parse(tcpInfo.mutable_json());
    if (!result) {
        LOG(ERROR) << "Failed to parse " << connection_file;
        return NULL;
    }

    Kernel *kernel  = new Kernel(number_io_threads, tcpInfo);
    return reinterpret_cast<kernel_t*>(kernel);
}

void free_kernel(kernel_t* k) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    delete kernel;
}


int kernel_start(kernel_t* k) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    kernel->start();
    return 0;
}

int kernel_shutdown(kernel_t* k) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    kernel->shutdown();
    return 0;
}

int kernel_has_shutdown(kernel_t* k) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    return kernel->has_shutdown();
}


void kernel_set_service_handler(kernel_t* k, shell_handler_t* s) {
    ExecuteHandler * handler = reinterpret_cast<ExecuteHandler*>(s);
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    kernel->set_shell_handler(handler);
}

/**
 * IPython shell handler
 */
shell_handler_t *new_ipython_shell_handler(kernel_t * k) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    assert(kernel != NULL);

    IPythonShellHandler * ipython_handler = new IPythonShellHandler();
    kernel->set_shell_handler(ipython_handler);
    return reinterpret_cast<shell_handler_t*>(ipython_handler);
}

/*
void free_ipython_shell_handler(shell_handler_t*s) {
    ExecuteHandler * handler = reinterpret_cast<ExecuteHandler*>(s);
    IPythonShellHandler * ipython_handler = dynamic_cast<IPythonShellHandler*>(handler);
    delete ipython_handler;
}
*/

void ipython_shell_handler_set_handlers(shell_handler_t* s, const handler_table_t * handler) {
    ExecuteHandler * h = reinterpret_cast<ExecuteHandler*>(s);
    IPythonShellHandler * ipython_handler = dynamic_cast<IPythonShellHandler*>(h);
    assert(ipython_handler != NULL);
    ipython_handler->set_handlers(*handler);

}


ioredir_t * new_ioredir_stdout() {
    redirector * red = new redirector(STDOUT_FILENO);
    red->start();
    return reinterpret_cast<ioredir_t*>(red);
}
ioredir_t * new_ioredir(int fileno) {
    redirector * red = new redirector(fileno);
    red->start();
    return reinterpret_cast<ioredir_t*>(red);
}


int ioredir_receive(ioredir_t *io, char ** b, int * len)
{
    redirector * red = reinterpret_cast<redirector*>(io);
    std::string s;
    if (!red->receive(s)) {
        return 0;
    }
    *b = (char*) malloc(sizeof(char) * s.size() + 1);
    memcpy(*b, s.data(), s.size());
    (*b)[s.size()] = '\0';
    *len = s.size();
    return s.size();
}

void free_ioredir(ioredir_t*io) {
    redirector * red = reinterpret_cast<redirector*>(io);
    delete red;
}

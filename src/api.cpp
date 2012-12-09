
#include "api.h"
#include "kernel.h"
#include "json.h"
#include "ipython_message.h"

struct kernel_env_t {
    void * zmq_ctx;
};

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

    std::ifstream fs(connection_file)
    json::parser p(fs);
    bool result = p.parse(tcpInfo.mutable_json());
    if (!result) {
        LOG(ERROR) << "Failed to parse " << existing_file ;
        return 1;
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

int kernel_stop(kernel_t*) {
    Kernel *kernel = reinterpret_cast<Kernel*>(k);
    kernel->start();

}
void kernel_set_service_handler(kernel_t*, shell_handler_t*);

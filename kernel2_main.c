
#include "api.h"
#include <string.h>
#include <signal.h>
#include <assert.h>

kernel_t * kernel = 0;

int execute_request(void * ctx,
                    const ipython_execute_request_t * request,
                    ipython_execute_response_t * response) {

    response->successful = 1;
    response->media_type = strdup("text/plain");
    response->data = strdup(request->code);
    return 0;
}


void shutdown_kernel(int sig) {
    if (kernel)
        kernel_shutdown(kernel);
    signal(SIGINT, SIG_DFL);
}

int main(int argc, char**argv) {

    const char * file = 0;
    int i = 0;
    for (i = 1; i<argc; ++i) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            file = argv[i + 1];
            ++i;
        }
    }

    assert(file != NULL);

    kernel = new_kernel_with_connection_file(/*threads*/1, file);

    shell_handler_t * handler = new_ipython_shell_handler(kernel);
    handler_table_t table;
    table.context = NULL;
    table.generic = NULL;
    table.execute_request = &execute_request;
    ipython_shell_handler_set_handlers(handler, &table);

    kernel_start(kernel);

    signal(SIGINT, shutdown_kernel);

    while(!kernel_has_shutdown(kernel)) {
        sleep(1);
    }

    if (kernel) {
        free_kernel(kernel);
    }
}

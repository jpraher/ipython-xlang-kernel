/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __kernel_api__h__
#define __kernel_api__h__

typedef
struct ipython_message {
    char * msg_type;

    char * header;
    char * parent;
    char * metatdata;
    char * content;
} ipython_message_t;


typedef
struct ipython_execute_request {
    const char * code;

} ipython_execute_request_t;


typedef
struct ipython_execute_response {
    // maybe directly set
    // char * data_text_plain;
    int  successful;
    char * media_type;
    char * data;
} ipython_execute_response_t;




typedef int (*ServiceFunction)(void * ctx,
                               const ipython_message_t * request,
                               ipython_message_t       * response
                               );

typedef int (*ExecuteRequestFunction)(void * ctx,
                                      const ipython_execute_request_t * request,
                                      ipython_execute_response_t      * response
                                      );


// opaque structure
struct kernel_env_t;
struct kernel_t;
struct shell_handler_t;

#ifdef __cplusplus
extern "C" {
#endif
/*
  kernel_env_t* new_kernel_env();
  free_kernel_env(kernel_env_t*);
*/
kernel_t* new_kernel_with_conection_file(kernel_env_t* env, const char* connection_file);
void free_kernel(kernel_t*);

int kernel_start(kernel_t*);
int kernel_shutdown(kernel_t*);
void kernel_set_service_handler(kernel_t*, shell_handler_t*);


/**
 * IPython shell handler
 */
shell_handler_t *new_ipython_shell_handler();
void free_ipython_shell_handler(shell_handler_t*);
void ipython_shell_handler_ctx(shell_handler_t*);
void ipython_shell_handler_generic(shell_handler_t*,ServiceFunction);
void ipython_shell_handler_execute_request(shell_handler_t*,ExecuteRequestFunction);

#ifdef __cplusplus
}
#endif

#endif // kernel_api

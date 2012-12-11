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


typedef
struct handler_table {
  void *              context;
  ServiceFunction        generic;
  ExecuteRequestFunction execute_request;
} handler_table_t;



// opaque structure
// typedef struct kernel_env_t;
typedef struct kernel kernel_t;
typedef struct shell_handler shell_handler_t;

#ifdef __cplusplus
extern "C" {
#endif
/*
  kernel_env_t* new_kernel_env();
  free_kernel_env(kernel_env_t*);
*/
kernel_t* new_kernel_with_connection_file(/*kernel_env_t* env*/ int number_of_threads, const char* connection_file);
void free_kernel(kernel_t*);

int kernel_start(kernel_t*);
int kernel_shutdown(kernel_t*);
/*  void kernel_reset_service_handler(kernel_t*, shell_handler_t*); */
int kernel_has_shutdown(kernel_t*);

/**
 * IPython shell handler owned by this kernel
 */
shell_handler_t *new_ipython_shell_handler(kernel_t *t);
// void free_ipython_shell_handler(shell_handler_t*);
void ipython_shell_handler_set_handlers(shell_handler_t* s, const handler_table_t * handler);

#ifdef __cplusplus
}
#endif

#endif // kernel_api

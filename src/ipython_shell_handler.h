/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __ipython_shell_handler__h__
#define __ipython_shell_handler__h__

#include "kernel.h"
#include "ipython_message.h"
#include "delegate.h"
#include "scoped_ptr.h"
#include <iostream>

// begin C interface
#include "api.h"
// end C interface

template< >
inline
void delete_op<ipython_execute_response>(ipython_execute_response * p) {
    assert(p != NULL);
    if (p->media_type)
        free(p->media_type);
    if (p->data)
        free(p->data);
    free(p);
}



class IPythonShellHandler : public ExecuteHandler{
public:
    IPythonShellHandler();
    virtual ~IPythonShellHandler();

    void set_handlers(const handler_table_t &handlers);

    virtual Message * create_request();
    virtual void execute(EContext & ctx,
                         Message *request);

    void handle_generic        (EContext & ctx, IPythonMessage * request);
    void handle_execute_request(EContext & ctx, IPythonMessage * request);

private:

    void _sendStream(EContext & ctx,
                     const json::object_value &parent_header,
                     const std::string &name,
                     EContext::get_string_t,
                     /* maybe put this in context */
                     std::ostream & os,
                     FILE * fstream
                     );

    void send_stdout_and_err(EContext & ctx,
                             IPythonMessage * request);

    size_t _execution_count;

    handler_table_t _handlers;

};

#endif // __ipython_shell_handler__h__

/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "ipython_shell_handler.h"
#include <glog/logging.h>



Message * IPythonShellHandler::create_request() {
    return new IPythonMessage();
}


std::string _topic(const std::string & ident, const std::string &msg_type) {
    return "kernel." + ident + "." + msg_type;
}


void _wrap(const IPythonMessage & msg, ipython_message_t * output) {

}






IPythonShellHandler::IPythonShellHandler() {

}

IPythonShellHandler::~IPythonShellHandler() {
}

void IPythonShellHandler::context(void* ctx)
{
    _ctx = ctx;
}

void IPythonShellHandler::execute_request_handler(ExecuteRequestFunction erf)
{
    _execute_request_function = erf;
}

void IPythonShellHandler::generic_handler(ServiceFunction sf)
{
    _service_function = sf;
}

void IPythonShellHandler::handle_generic(EContext & ctx,
                                         IPythonMessage * request)
{
    /* generic failure */
    if (request == NULL) throw std::exception();

    const std::string & msg_type = json::get(request->header.string("msg_type"), json::value::EMPTY_STRING);

    size_t  pos_dash = msg_type.rfind('_');
    if (pos_dash != std::string::npos && msg_type.substr(pos_dash + 1) == "request") {
        DLOG(INFO) << "received **_request";

        std::string response_msg_type = msg_type.substr(0, pos_dash + 1) + "reply";

        IPythonMessage response;

        response.idents = request->idents;
        response.content.set_string("status", "AccessError");

        // get millis since
        response.metadata.set_boolean("dependencies_met", true);
        // TODO set ident
        response.metadata.set_string("engine", ctx.ident());

        time_t msnow = time((time_t*)NULL) * 1000;
        response.metadata.set_int64("started",  msnow);
        response.metadata.merge(request->metadata);

        // always new msgid ...
        response.header.set_string("msg_id", _generate_uuid());

        if (request->header.string("username")) {
            response.header.set_string("username", *request->header.string("username"));
        }

        response.header.set_string("msg_type", response_msg_type);
        if (request->header.string("session")) {
            response.header.set_string("session", *request->header.string("session"));
        }

        response.parent.merge(request->header);
        ctx.shell().send(response);
    }
    else {
        LOG(ERROR) << "Failed to handle generic reply.";
        send_stdout_and_err(ctx, request);
    }

    send_stdout_and_err(ctx, request);
}

void IPythonShellHandler::_sendStream(EContext & ctx,
                                      const json::object_value &parent_header,
                                      const std::string &name,
                                      EContext::get_string_t get_string,
                                      /* maybe put this in context */
                                      std::ostream & os,
                                      FILE * fstream
                                      )
{
    os.flush();
    fflush(fstream);
    IPythonMessage pyout;
    pyout.idents.push_back(_topic(ctx.ident(), "stream"));
    pyout.parent.merge(parent_header);
    pyout.header.set_string("msg_id", _generate_uuid());
    pyout.header.set_string("msg_type", "stream");
    pyout.header.set_string("session", ctx.session_id());
    pyout.content.set_string("data", (ctx.*get_string)());
    pyout.content.set_string("name", "stderr");
    ctx.io().send(pyout);
}

void IPythonShellHandler::send_stdout_and_err(EContext & ctx,
                                              IPythonMessage * request)
{
    _sendStream(ctx,
                request->header,
                "stdout",
                &EContext::stdout,
                std::cout,
                stdout
                );

    _sendStream(ctx,
                request->header,
                "stderr",
                &EContext::stderr,
                std::cerr,
                stderr
                );
}


void IPythonShellHandler::handle_execute_request(EContext & ctx,
                                                 IPythonMessage * request)
{

    // get info if we are silent
    bool silent = json::get(request->content.boolean("silent"), false);

    const std::string *codePtr = request->content.string("code");
    std::string code;
    if (codePtr) {
        code = *codePtr;
    }
    // trivial RTrim
    while (code.size() > 0 && isspace(code[code.size() - 1])) {
        code = code.substr(0, code.size() - 1);
    }

    /*
     */

    if (!silent) {
        IPythonMessage pyin;
        pyin.idents.push_back(_topic(ctx.ident(), "pyin"));
        pyin.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyin.header.set_string("msg_id",oss.str());
        }
        pyin.header.set_string("msg_type", "pyin");
        pyin.header.set_string("session", ctx.session_id());
        pyin.content.set_int64("execution_count", _execution_count);

        json::object_value * pyout_data = pyin.content.mutable_object("code");
        pyin.content.set_string("code", json::get(request->content.string("code"), json::value::EMPTY_STRING));
        std::cout << "PYIN content >" << pyin.content.to_str() << std::endl;
        ctx.io().send(pyin);
    }


    ipython_execute_request_t execute_request;
    scoped_ptr<ipython_execute_response_t>  execute_response(alloc_init<ipython_execute_response_t>());
    memset(execute_response.get(), 0, sizeof(execute_response));
    execute_request.code = code.c_str();
    if (_execute_request_function) {
        _execute_request_function(_ctx, &execute_request, execute_response.get());
    }


    IPythonMessage response;

    if (execute_response->successful) {
        response.idents = request->idents;
        response.content.set_string("status", "ok");
        response.content.set_int64("execution_count", ++_execution_count);

        // ok case
        response.content.mutable_array("payload");
        response.content.mutable_object("user_variables");
        response.content.mutable_object("user_expressions");

        // get millis since
        response.metadata.set_boolean("dependencies_met", true);
        // TODO set ident
        response.metadata.set_string("engine", ctx.ident());

        time_t msnow = time((time_t*)NULL) * 1000;
        response.metadata.set_int64("started",  msnow);
        response.metadata.merge(request->metadata);

        // always new msgid ...

        response.metadata.set_string("status", "ok");
        // {"date":"2012-11-27T21:55:17.543998","msg_id":"a9522f9c-f097-4921-9d28-35688bd7e32a","msg_type":"execute_request","session":"2efc817e-52d4-4945-aa36-93b755f238fb","username":"jakob","version":[0,14,0,"dev"]}

        response.header.set_string("msg_id", _generate_uuid());

        if (request->header.string("username")) {
            response.header.set_string("username", *request->header.string("username"));
        }
        //if (request.header.string("msg_type")) {
        response.header.set_string("msg_type", "execute_reply");
        // *request.header.string("msg_type"));
        //}
        if (request->header.string("session")) {
            response.header.set_string("session", *request->header.string("session"));
        }

        response.parent.merge(request->header);
        /*
        std::cout << "RESPONSE header  >" << response.header.to_str() << std::endl;
        std::cout << "RESPONSE content >" << response.content.to_str() << std::endl;
        std::cout << "RESPONSE metadata>" <<  response.metadata.to_str() << std::endl;
        */
    }
    else {     //  error

    }

    ctx.shell().send(response);

    if (!silent && execute_response->successful) {
        IPythonMessage pyout;
        pyout.idents.push_back(_topic(ctx.ident(), "pyout"));
        pyout.parent.merge(request->header);
        pyout.header.set_string("msg_id", _generate_uuid());
        pyout.header.set_string("msg_type", "pyout");
        pyout.header.set_string("session", ctx.session_id());
        pyout.content.set_int64("execution_count", _execution_count);

        json::object_value * pyout_data = pyout.content.mutable_object("data");
        if (::strcmp(execute_response->media_type, "text/json") == 0) {
            // set the whole data
        }
        else {
            pyout_data->set_string(execute_response->media_type, execute_response->data);
        }
        std::cout << "PYOUT content >" << pyout.content.to_str() << std::endl;
        ctx.io().send(pyout);
    }

    if (!silent) {
        send_stdout_and_err(ctx, request);
    }
    // free request.
}



void IPythonShellHandler::execute(EContext & ctx,
                                  Message *requestMsg)
{
    DLOG(INFO) << "IPythonShellHandler::execute()";

    IPythonMessage * request = dynamic_cast<IPythonMessage*>(requestMsg);
    assert(request != NULL);

    std::cout << "REQUEST header>" <<  request->header.to_str() << std::endl;
    std::cout << "REQUEST content>" <<  request->content.to_str() << std::endl;
    std::cout << "REQUEST metadata>" <<  request->metadata.to_str() << std::endl;

    std::string msg_type = json::get(request->header.string("msg_type"), json::value::EMPTY_STRING);
    //const std::map<std::string,
    if (msg_type == "execute_request") {
        handle_execute_request(ctx, request);
    }
    else {
        handle_generic(ctx, request);
    }
}

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



IPythonShellHandler::IPythonShellHandler(Kernel * kernel)
    :_execution_count(0),
     _kernel(kernel)
{

}

IPythonShellHandler::~IPythonShellHandler() {
}

void IPythonShellHandler::set_handlers(const handler_table_t &handlers) {
    DLOG(INFO) << "set_handlers" ;
     _handlers = handlers;
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
    pyout.content.set_string("name", name);
    DLOG(INFO) << "PYOUT: " << name;
    DLOG(INFO) << pyout.content.to_str();
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
    DLOG(INFO) << "Entering handle_execute_request";
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
        DLOG(INFO) << "PYIN content >" << pyin.content.to_str() ;
        ctx.io().send(pyin);
    }


    ipython_execute_request_t execute_request;
    scoped_ptr<ipython_execute_response_t>  execute_response(alloc_init<ipython_execute_response_t>());
    memset(&execute_request, 0, sizeof(execute_request));
    execute_request.code = code.c_str();
    std::string content_string = request->content.to_str();
    execute_request.content_json_string = content_string.c_str();
    execute_response->status = StatusError;
    if (_handlers.execute_request != NULL) {
        _handlers.execute_request(_handlers.context,
                                  &execute_request,
                                  execute_response.get());
    }


    IPythonMessage response;
    DLOG(INFO) << "Got response " << execute_response->status ;

    // TODO add `abort` information => enum!
    std::string status = execute_response->status == StatusOk ? "ok" : "error";

    if (!silent) {
        ++_execution_count;
    }
    send_stdout_and_err(ctx, request);

    DLOG(INFO) << "response status " << status;

    response.idents = request->idents;
    response.content.set_string("status", status);
    response.content.set_int64("execution_count", _execution_count);

    // ok case
    if (execute_response->status == StatusOk) {
        response.content.mutable_array("payload");
        response.content.mutable_object("user_variables");
        response.content.mutable_object("user_expressions");
    } else if (execute_response->status == StatusError){
        // ename string  exception name
        // evalue string exception value
        // traceback : string list
        if (execute_response->exception_name) {
            response.content.set_string("ename", execute_response->exception_name);
        }
        else {
            response.content.set_string("ename", "");
        }
        if (execute_response->exception_value) {
            response.content.set_string("evalue", execute_response->exception_value);
        }
        else {
            response.content.set_string("evalue", "");
        }
        json::array_value * value = response.content.mutable_array("traceback");
        if (execute_response->traceback && execute_response->traceback_len > 0 ) {
            for (int i = 0 ; i < execute_response->traceback_len; ++i) {
                value->set_string(i, execute_response->traceback[i]);
            }
        }
    }

    // get millis since
    response.metadata.set_boolean("dependencies_met", true);
    // TODO set ident
    response.metadata.set_string("engine", ctx.ident());

    time_t msnow = time((time_t*)NULL) * 1000;
    response.metadata.set_int64("started",  msnow);
    response.metadata.merge(request->metadata);

    // response.metadata.set_string("status", "ok");
    // {"date":"2012-11-27T21:55:17.543998","msg_id":"a9522f9c-f097-4921-9d28-35688bd7e32a","msg_type":"execute_request","session":"2efc817e-52d4-4945-aa36-93b755f238fb","username":"jakob","version":[0,14,0,"dev"]}

    // always new msgid ...
    response.header.set_string("msg_id", _generate_uuid());

    if (request->header.string("username")) {
        response.header.set_string("username", *request->header.string("username"));
    }

    response.header.set_string("msg_type", "execute_reply");

    if (request->header.string("session")) {
        response.header.set_string("session", *request->header.string("session"));
    }

    response.parent.merge(request->header);

    DLOG(INFO) << "RESPONSE header  >" << response.header.to_str();
    DLOG(INFO) << "RESPONSE content >" << response.content.to_str();
    DLOG(INFO) << "RESPONSE metadata>" <<  response.metadata.to_str();

    ctx.shell().send(response);

    if (!silent && execute_response->status == StatusOk) {
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
        DLOG(INFO) << "PYOUT content >" << pyout.content.to_str();
        ctx.io().send(pyout);
    }


    // free request.
}

void IPythonShellHandler::handle_shutdown_request(EContext &ctx,  IPythonMessage * request)
{

    bool restart = json::get(request->content.boolean("restart"), false);
    IPythonMessage response;
    response.metadata.merge(request->metadata);
    response.header.set_string("msg_id", _generate_uuid());
    if (request->header.string("username")) {
        response.header.set_string("username", *request->header.string("username"));
    }

    response.header.set_string("msg_type", "shutdown_reply");
    if (request->header.string("session")) {
        response.header.set_string("session", *request->header.string("session"));
    }

    // response.parent = request.header
    response.parent.merge(request->header);

    response.content.set_boolean("restart",restart);

    DLOG(INFO) << "RESPONSE header  >" << response.header.to_str();
    DLOG(INFO) << "RESPONSE content >" << response.content.to_str();
    DLOG(INFO) << "RESPONSE metadata>" <<  response.metadata.to_str();
    send_stdout_and_err(ctx, request);
    ctx.shell().send(response);

    DLOG(INFO) << "Stopping kernel" ;
    _kernel->stop();
    DLOG(INFO) << "Kernel stopped" ;
    if (!restart) {
        _kernel->shutdown();
        DLOG(INFO) << "Has kernel shutdown: " << _kernel->has_shutdown();
    }
    else {
        _kernel->start();
        DLOG(INFO) << "Kernel restarted.";
    }
    google::FlushLogFiles(google::INFO);
}


void IPythonShellHandler::execute(EContext & ctx,
                                  Message *requestMsg)
{
    DLOG(INFO) << "IPythonShellHandler::execute()";

    IPythonMessage * request = dynamic_cast<IPythonMessage*>(requestMsg);
    assert(request != NULL);

    DLOG(INFO) << "REQUEST header>" <<  request->header.to_str() ;
    DLOG(INFO) << "REQUEST content>" <<  request->content.to_str() ;
    DLOG(INFO) << "REQUEST metadata>" <<  request->metadata.to_str() ;

    std::string msg_type = json::get(request->header.string("msg_type"), json::value::EMPTY_STRING);

    if (msg_type == "execute_request") {
        handle_execute_request(ctx, request);
    }
    else if (msg_type == "shutdown_request") {
        handle_shutdown_request(ctx, request);
    }
    else {
        handle_generic(ctx, request);
    }
}

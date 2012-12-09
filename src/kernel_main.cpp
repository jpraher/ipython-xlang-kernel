/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */


#include "kernel.h"
#include "ipython_message.h"
#include "hmac.h"
#include "tthread/tinythread.h"
#include "delegate.h"
#include "ipython_shell_handler.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <map>
#include <uuid/uuid.h>
#include <glog/logging.h>
#include <time.h>



std::string _topic(const std::string & ident, const std::string &msg_type) {
    return "kernel." + ident + "." + msg_type;
}

class PrintOutCallback : public ExecuteHandler{
public:
    PrintOutCallback();
    virtual ~PrintOutCallback();

    virtual Message * create_request();
    virtual void execute(EContext & ctx,
                         Message *request);
private:
    size_t _execution_count;
};

PrintOutCallback::PrintOutCallback()
    : _execution_count(0)
{
}

PrintOutCallback::~PrintOutCallback() {}

Message * PrintOutCallback::create_request() {
    return new IPythonMessage();
}
void PrintOutCallback::execute(EContext & ctx,
                               Message *requestMsg)
{
    DLOG(INFO) << "PrintOutCallback::handle()";

    IPythonMessage * request = dynamic_cast<IPythonMessage*>(requestMsg);
    assert(request != NULL);

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
    std::cout << "REQUEST header>" <<  request->header.to_str() << std::endl;
    std::cout << "REQUEST content>" <<  request->content.to_str() << std::endl;
    std::cout << "REQUEST metadata>" <<  request->metadata.to_str() << std::endl;

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



    IPythonMessage response;

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

    {
        uuid_t msg_id;
        uuid_generate(msg_id);
        std::ostringstream oss;
        _uuid_stringify(msg_id, oss);
        response.header.set_string("msg_id",oss.str());
    }
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
    std::cout << "RESPONSE header  >" << response.header.to_str() << std::endl;
    std::cout << "RESPONSE content >" << response.content.to_str() << std::endl;
    std::cout << "RESPONSE metadata>" << response.metadata.to_str() << std::endl;

    ctx.shell().send(response);

    if (!silent && !code.empty()) {
        IPythonMessage pyout;
        pyout.idents.push_back(_topic(ctx.ident(), "pyout"));
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "pyout");
        pyout.header.set_string("session", ctx.session_id());
        pyout.content.set_int64("execution_count", _execution_count);

        json::object_value * pyout_data = pyout.content.mutable_object("data");
        pyout_data->set_string("text/plain", code);
        std::cout << "PYOUT content >" << pyout.content.to_str() << std::endl;
        ctx.io().send(pyout);
    }

    if (!silent) {
        std::cout.flush();
        fflush(stdout);
        IPythonMessage pyout;
        pyout.idents.push_back(_topic(ctx.ident(), "stream"));
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "stream");
        pyout.header.set_string("session", ctx.session_id());
        pyout.content.set_string("data", ctx.stdout());
        pyout.content.set_string("name", "stdout");
        ctx.io().send(pyout);
    }
    if (!silent) {
        std::cerr.flush();
        fflush(stderr);
        IPythonMessage pyout;
        pyout.idents.push_back(_topic(ctx.ident(), "stream"));
        pyout.parent.merge(request->header);
        {
            uuid_t msg_id;
            uuid_generate(msg_id);
            std::ostringstream oss;
            _uuid_stringify(msg_id, oss);
            pyout.header.set_string("msg_id",oss.str());
        }
        pyout.header.set_string("msg_type", "stream");
        pyout.header.set_string("session", ctx.session_id());
        pyout.content.set_string("data", ctx.stderr());
        pyout.content.set_string("name", "stderr");
        ctx.io().send(pyout);
    }
    // free request.
}

/*
typedef int (*ExecuteRequestFunction)(void * ctx,
                                      const ipython_execute_request_t * request,
                                      ipython_execute_response_t * response
                                      );
*/
int execute_request(void * ctx,
                    const ipython_execute_request_t * request,
                    ipython_execute_response_t * response) {

    response->successful = true;
    response->media_type = strdup("text/plain");
    response->data = strdup(request->code);
    return 0;
}


int main(int argc, char** argv) {

    google::InitGoogleLogging(argv[0]);

    /*
starting kernel {'extra_arguments': [u"--KernelApp.parent_appname='ipython-notebook'"], 'cwd': u'/Users/jakob/extsrc/ipython'}
Starting new kernel af0f4df7-3741-4ec5-9d6a-74c1142dcff6
/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-3741-4ec5-9d6a-74c1142dcff6.json
ipkernel launch_kernel: args (), kwargs {'extra_arguments': [u"--KernelApp.parent_appname='ipython-notebook'"], 'cwd': u'/Users/jakob/extsrc/ipython', 'fname': u'/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-3741-4ec5-9d6a-74c1142dcff6.json'}
entry_point - launching kernel ['/usr/bin/python', '-c', 'from IPython.zmq.ipkernel import main; main()', '-f', u'/Users/jakob/.ipython/profile_default/security/kernel-af0f4df7-
     */

    // '-f', u'/Users/jakob/.ipython/profile_default/security/kernel-524ecbc8-3573-424c-ab97-77f9154b1c7b.json'

    int io_threads = 1;
    std::string ip = "127.0.0.1";
    zmq::context_t ctx(io_threads);

    DLOG(INFO) << "Starting kernel";
    std::string id;
    std::string existing_file;
    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (::strcmp("--id", arg) == 0 && i + 1 < argc) {
            id = argv[++i];
            std::cout << "reusing id " << id << std::endl;
        }
        else if (::strcmp("-f", arg) == 0 && i + 1 < argc) {
            existing_file = argv[++i];
            DLOG(INFO)<< "existing configuration " << existing_file ;
        }
    }

    Kernel::TCPInfo tcpInfo;
    if (!existing_file.empty()) {
        std::ifstream fs(existing_file.c_str());
        json::parser p(fs);
        bool result = p.parse(tcpInfo.mutable_json());
        if (!result) {
            // failed to parse
            LOG(ERROR) << "Failed to parse " << existing_file ;
            return 1;
        }
    } else {

        uuid_t kernelid;
        if (id.empty()) {
            uuid_generate(kernelid);
        }
        else {
            std::istringstream is(id);
            _uuid_parse(is, kernelid);
            std::ostringstream os;
            _uuid_stringify(kernelid, os);
            assert( os.str() == id );
        }

        std::string key_string;
        std::string key_bin;
        {
            uuid_t key;
            uuid_generate(key);
            key_bin = std::string((char*)key, sizeof(key));
            std::ostringstream uuidstr;
            _uuid_stringify(key, uuidstr);
            key_string = uuidstr.str();
        }
    }

    IPythonShellHandler shellHandler;
    shellHandler.context(NULL);
    shellHandler.execute_request_handler(execute_request);

    Kernel kernel(ctx, tcpInfo,  &shellHandler);
    DLOG(INFO) << "Kernel id " << kernel.ident();
    try {
        kernel.start();
    } catch (const std::exception & e) {
        LOG(ERROR) << "Failed to start kernel: " << e.what();
        return 2;
    }

    if (existing_file.empty()) {
        std::ostringstream connectionfile;
        connectionfile << "kernel-" << kernel.ident() << ".json";
        {
            std::ofstream fs(connectionfile.str().c_str());
            kernel.endpoint_info().json().stringify(fs);
        }
        std::cout << "wrote " << connectionfile.str() << std::endl;
    }
    kernel.message_loop();

}

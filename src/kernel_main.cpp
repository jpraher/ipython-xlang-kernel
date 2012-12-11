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
#include <signal.h>
#include <cassert>

std::string _topic(const std::string & ident, const std::string &msg_type) {
    return "kernel." + ident + "." + msg_type;
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

Kernel * kernel = NULL;


void shutdown_kernel(int sig) {
    LOG(INFO) << "Ctrl-C catched, shutting down.";
    kernel->shutdown();
    LOG(INFO) << "Ctrl-C catched, shutted down.";
    signal(SIGINT, SIG_DFL);
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
    handler_table_t handlers;
    handlers.context = NULL;
    handlers.execute_request = execute_request;
    handlers.generic = NULL;

    //shellHandler.context(NULL);
    // shellHandler.execute_request_handler(execute_request);


    kernel = new Kernel(io_threads, tcpInfo);
    kernel->set_shell_handler(&shellHandler);
    DLOG(INFO) << "Kernel id " << kernel->ident();
    try {
        kernel->start();
    } catch (const std::exception & e) {
        LOG(ERROR) << "Failed to start kernel: " << e.what();
        return 2;
    }

    if (existing_file.empty()) {
        std::ostringstream connectionfile;
        connectionfile << "kernel-" << kernel->ident() << ".json";
        {
            std::ofstream fs(connectionfile.str().c_str());
            kernel->endpoint_info().json().stringify(fs);
        }
        std::cout << "wrote " << connectionfile.str() << std::endl;
    }

    signal(SIGINT, shutdown_kernel);
    DLOG(INFO) << "Waiting for shutting down.";
    while(!kernel->has_shutdown()) {
        sleep(1);
    }
    DLOG(INFO) << "Shutted down.";
    delete kernel;
}

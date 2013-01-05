
#include "tthread/tinythread.h"
#include "delegate.h"
#include "json.h"
#include "ioredir.h"
#include <glog/logging.h>
#include <zmq.hpp>
#include <iostream>
#include <fcntl.h>

redirector::redirector(int fd /*, function1<void,const std::string&> target*/)
    : _redirect(delegate_t<redirector>(this, &redirector::_do_redirect)),
      _stopped(true),
      _fd(fd),
      _fd_orig(-1)
      // _target(target)
{
    _filedes[READ] = -1;
    _filedes[WRITE] = -1;
}

redirector::~redirector() {
    stop();
}

// int redirector::fd_orig() { return _fd_orig; }

void redirector::_do_redirect() {
   while (!_stopped) {
       zmq::pollitem_t items[1];
       const int NUM_ITEMS = sizeof(items)/ sizeof(zmq::pollitem_t);

       // _reading = false;
       items[0].fd = _filedes[READ];
       items[0].socket = NULL;
       items[0].events = ZMQ_POLLIN;

       // DLOG(INFO) << "before zmq_pool";
       int result = zmq_poll(items, NUM_ITEMS, 5000 );
       if (result < 0) {
           LOG(WARNING) << "Failed to call zmq_poll " << result;
       }
       // DLOG(INFO) << "IO zmq_poll returned " << result;
       if (result ==  0) {
           continue;
       }
       bool ok  = _receive(_receive_mutex, _data, _filedes[READ], 0);
   }
}


bool redirector::receive(std::string & s) {
    tthread::lock_guard<tthread::recursive_mutex> guard(_receive_mutex);
    bool result = _receive(_receive_mutex, _data, _filedes[READ], 0);
    s = _data;
    _data.clear();
    return result;
}

bool redirector::_receive(tthread::recursive_mutex & mutex, std::string & s, int fd, int timeout) {

    tthread::lock_guard<tthread::recursive_mutex> guard(mutex);

    zmq::pollitem_t items[1];
    const int NUM_ITEMS = sizeof(items)/ sizeof(zmq::pollitem_t);

    // _reading = false;
    items[0].fd = fd;
    items[0].socket = NULL;
    items[0].events = ZMQ_POLLIN;

    // DLOG(INFO) << "before zmq_pool";
    int result = zmq_poll(items, NUM_ITEMS, timeout);
    if (result < 0) {
        LOG(WARNING) << "Failed to call zmq_poll " << result;
        return false;
    }
    // DLOG(INFO) << "IO zmq_poll returned " << result;
    if (result ==  0) {
        return true;
    }
    // _reading = true;
    // DLOG(INFO) << "IO received" ;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        if (items[i].revents & ZMQ_POLLIN) {
            const int BUF_SIZE = 1024;
            char data[BUF_SIZE];
            int res = 0;
            do {
                res = read(items[i].fd, data, sizeof(data));
                if (res > 0) {
                    s.append(data, res);
                    // DLOG(INFO) << "IO data:" << s ;
                }
            } while (res == BUF_SIZE);
        }
    }
    // DLOG(INFO) << "IO done" ;
    return true;
}

void redirector::start() {

    if (_stopped) {
        int result = pipe(_filedes);
        if (result != 0) {
            LOG(WARNING) << "Failed to create pipe return " << result;
            return;
        }

        // set read descriptor to be non blocking
        int flags = fcntl(_filedes[READ], F_GETFL);
        if (fcntl(_filedes[READ], F_SETFL,flags | O_NONBLOCK ) < 0) {
            LOG(WARNING) << "Failed to set read descriptor to nonblocking.";
            return;
        }
        // this is the magic in the kernel to alias
        // the two file descriptors
        // filedes[WRITE] -> _filedes
        _fd_orig = dup(_fd);
        dup2(_filedes[WRITE], _fd);
        close(_filedes[WRITE]);
        _filedes[WRITE] = -1;
    }

    _stopped = false;
    if (!_redirect.start()) {
        LOG(WARNING) << "Starting redirection failed.";
    }
}

void redirector::stop() {

    if (_stopped) return;

    if (_filedes[READ] != -1) {
        close(_filedes[READ]);
        _filedes[READ] = -1;
    }
    // restore
    if (_fd_orig != -1) {
        dup2(_fd, _fd_orig);
        close(_fd_orig);
        _fd_orig = -1;
    }

    _stopped = true;
    _redirect.join();
}


/*

struct writeout {
     void write(const std::string & s) ;
};

void writeout::write(const std::string &s ) {
    std::cerr << "WO REDIR: " << json::string_value(s).to_str() << std::endl;
}

void _do_out(const std::string & s) {
    std::cerr << "REDIR: " << json::string_value(s).to_str() << std::endl;
}

int main(int argc, char** argv) {

    writeout w;
    delegate1_t<writeout, void, const std::string&> d(&w, &writeout::write);
    d("test");
    int fd = fileno(stdout);
    std::cout << fd << std::endl;
    std::cout << STDOUT_FILENO << std::endl;
    redirector redir(STDOUT_FILENO, d);
    redir.start();


    // d("test");
    function1<void, const std::string &> s(d);
    s("test");
    function1<void, const std::string &> s2(&_do_out);

    printf("test\n");
    std::cout << "test" << std::endl;
    std::cout.flush();
    fflush(stdout);

    sleep(1);
}
*/

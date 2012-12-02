
#ifndef __redirect__h__
#define __redirect__h__

#include "tthread/tinythread.h"
#include "delegate.h"

class redirector {
public:
    static const size_t READ = 0;
    static const size_t WRITE = 1;

//redirector(int fd, const function1_t<void,const std::string&>
//&target);
    redirector(int fd, function1<void, const std::string &> target);
    ~redirector();
    void start();
    void stop();

private:
    void _do_redirect();

    active_method<redirector> _redirect;
    volatile bool _stopped;
    int _fd;
    int _fd_orig;

    tthread::thread * redir;
    int _filedes[2];
    function1<void,const std::string&> _target;

};

#endif

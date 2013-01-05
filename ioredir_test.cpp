

#include "ioredir.h"
#include "api.h"
#include <cassert>
#include <cstdio>



int main(int argc, char ** argv)
{

    const char * DATA =  "12123123123123123123123123123123123121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231121231231231231231231231231231231231";


    ioredir_t * io = new_ioredir_stdout();
    // redirector red(STDOUT_FILENO);
    // red.start();

    fprintf(stdout, DATA);
    /* fprintf(stdout, DATA);
    fprintf(stdout, DATA);
    fprintf(stdout, DATA);
    fprintf(stdout, DATA); */
    fflush(stdout);


    char * buf ;
    int len ;
    ioredir_receive(io, &buf, &len);
    std::string s(buf, len);
    /*
    std::string s;

    red.receive(s);
    */
    assert(s.substr(0, strlen(DATA)) == DATA);
    free_ioredir(io);
}

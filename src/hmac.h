
#ifndef __hmac__h__
#define __hmac__h__

#include <openssl/hmac.h>
#include <string>

namespace md {

class hmac_ctx_t {
 public:
    hmac_ctx_t();
    ~hmac_ctx_t();

     HMAC_CTX * get() {return &_ctx;}

    operator HMAC_CTX* ();
 private:
    HMAC_CTX _ctx;
};

class HMAC {
 public:
    HMAC(hmac_ctx_t & ctx, const std::string & key);
    ~HMAC();

    void update(const std::string & data);
    void update(const char * data, size_t len);
    void final(std::string * md);

 private:
    hmac_ctx_t & _ctx;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    std::string _key;
};

}

#endif

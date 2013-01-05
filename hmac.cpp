/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#include "hmac.h"
#include <openssl/hmac.h>


using md::hmac_ctx_t;
// using md::HMAC;

hmac_ctx_t::hmac_ctx_t() {
    HMAC_CTX_init(&_ctx);
}
hmac_ctx_t::~hmac_ctx_t() {
    HMAC_CTX_cleanup(&_ctx);
}

hmac_ctx_t::operator HMAC_CTX*() {
    return &_ctx;
}


md::HMAC::HMAC(hmac_ctx_t & ctx, const std::string & key)
    : _ctx(ctx)
{
    HMAC_Init_ex(_ctx.get(), (void*)key.data(), key.size(), EVP_md5(), NULL);
}

md::HMAC::~HMAC() {
}

void md::HMAC::update(const std::string & data) {
    HMAC_Update(_ctx, (unsigned char*)data.data(), data.size());
}

void md::HMAC::update(const char * data, size_t size) {
    HMAC_Update(_ctx, (unsigned char*)data, size);
}

void md::HMAC::final(std::string * md)
{
    unsigned int size;
    HMAC_Final(_ctx, md_value, &size);
    *md = std::string((char*)md_value, size);
}

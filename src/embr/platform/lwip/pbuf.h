/**
 * @file
 * 
 * Where netbuf-mk2 pbuf provider lives
 * 
 * Adapted from bronze-star PbufNetbufWrapper
 */

#pragma once

extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#include <estd/internal/platform.h>
#include <embr/netbuf.h>

#undef putchar
#undef puts
#undef putc

// CONFIG_PBUF_CHAIN could be coming from esp-idf
#ifdef CONFIG_PBUF_CHAIN
#define FEATURE_EMBR_PBUF_CHAIN_EXP
#endif


namespace embr { namespace lwip {

struct PbufNetbufBase
{
    typedef struct pbuf pbuf_type;
    typedef pbuf_type* pbuf_pointer;

private:
};

// netbuf-mk2 managing a lwip pbuf
struct PbufNetbuf : PbufNetbufBase
{

private:
    pbuf_pointer p;
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
    pbuf_pointer p_start;
#endif

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    PbufNetbuf(size_type size)
    {
        p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

    PbufNetbuf(pbuf_pointer p, bool bump_reference = true) : p(p)
    {
        if(bump_reference) pbuf_ref(p);

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    PbufNetbuf(PbufNetbuf&& move_from) :
        p(move_from.p)
    {
        move_from.p = NULLPTR;
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
        p_start = p;
#endif
    }
#endif

    ~PbufNetbuf()
    {
        if(p != NULLPTR)
            // remember, pbufs are reference counted so this may or may not actually
            // deallocate pbuf memory
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
            // NOTE: Unsure how pbuf_free works on a chain of pbufs
            pbuf_free(p_start);
#else
            pbuf_free(p);
#endif
    }

    // p->len represents length of current pbuf, if a chain is involved
    // look at tot_len
    size_type size() const { return p->len; }

    size_type total_size() const { return p->tot_len; }

    uint8_t* data() const { return (uint8_t*) p->payload; }

    bool next() { return false; }

    embr::mem::ExpandResult expand(size_type by_size, bool move_to_next)
    { 
        return embr::mem::ExpandFailFixedSize;
    }

    // EXPERIMENTAL
    void shrink(size_type to_size)
    {
        pbuf_realloc(
#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
            p_start, 
#else
            p,
#endif
            to_size);
    }
};

}}
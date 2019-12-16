#pragma once

#include "udp.h"
#include "streambuf.h"

namespace embr { namespace lwip { namespace experimental {


struct TransportBase
{
    typedef const ip_addr_t* addr_pointer;
    typedef struct pbuf* pbuf_pointer;

    struct Endpoint
    {
        addr_pointer address;
        uint16_t port;
    };

    typedef Endpoint endpoint_type;

};

struct TransportUdp : 
    TransportBase
{
    Pcb pcb;

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    TransportUdp(TArgs&& ...args) : pcb(std::forward<TArgs>(args)...) {}
#endif

    template <class TChar>
    void send(out_pbuf_streambuf<TChar>& streambuf, const endpoint_type& endpoint)
    {
        pcb.send(streambuf.netbuf().pbuf(), 
            endpoint.address,
            endpoint.port);
    }
};

}}}
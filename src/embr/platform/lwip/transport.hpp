#include "transport.h"
#include "../../events.h"

namespace embr { namespace lwip { namespace experimental {

template <class TDataPort>
UdpDataportTransport::UdpDataportTransport(TDataPort* dataport, uint16_t port)
{
    /* get new pcb */
    struct udp_pcb* pcb = udp_new();
    
    if (pcb == NULL) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return;
    }

    /* bind to any IP address on specified port */
    if (udp_bind(pcb, IP_ADDR_ANY, port) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        return;
    }

    /* set data_recv() as callback function
       for received packets */
    udp_recv(pcb, data_recv<TDataPort>, dataport);

    // allocate second one exclusively for send operations
    this->pcb.alloc();
}


template <class TDataPort>
void UdpDataportTransport::data_recv(void *arg, 
    struct udp_pcb *pcb, pbuf_pointer p,  
    addr_pointer addr, u16_t port)
{
    if (p != NULL) 
    {
        typedef TDataPort dataport_t;
        auto dataport = static_cast<dataport_t*>(arg);

        endpoint_type a(addr, port);
        // TODO: Be sure our PbufNetbuf aligns with transport_policy
        // not bumping ref because our 'move' later means we won't be deallocating
        // here, but rather when we dequeue
        embr::lwip::PbufNetbuf netbuf(p, false);

        dataport->notify(typename dataport_t::event::transport_received(netbuf, a));

        dataport->enqueue_from_receive(std::move(netbuf), a);
    }    
}


template <class TSubject>
Pcb UdpSubjectTransport::recv(TSubject& subject, uint16_t port)
{
    Pcb pcb;
    
    /* get new pcb */
    if (!pcb.alloc()) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return pcb;
    }

    /* bind to any IP address on specified port */
    if (pcb.bind(port) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        pcb.free();
        return pcb;
    }

    /* set data_recv() as callback function
       for received packets */
    pcb.recv(data_recv<TSubject>, &subject);

    return pcb;
}

template <class TSubject>
UdpSubjectTransport::UdpSubjectTransport(TSubject& subject, uint16_t port)
{
    recv(subject, port);

    // allocate second one exclusively for send operations
    this->pcb.alloc();
}


template <class TSubject>
void UdpSubjectTransport::data_recv(void *arg, 
    pcb_pointer pcb, pbuf_pointer p,  
    addr_pointer addr, u16_t port)
{
    if (p != NULL) 
    {
        typedef embr::event::Transport<base_type> event;

        auto subject = static_cast<TSubject*>(arg);

        endpoint_type a(addr, port);
        // TODO: Be sure our PbufNetbuf aligns with transport_policy
        // Not bumping ref.  If pbuf is desired to be kept past end of this
        // data_recv, the observer must bump the reference
        embr::lwip::PbufNetbuf netbuf(p, false);

        subject->notify(typename event::transport_received(netbuf, a));
    }    
}

}}}
#pragma once

#include <estd/type_traits.h>
#include <estd/allocators/fixed.h>

#include "../netbuf.h"

namespace embr {

namespace experimental {

// conforms to estd's extended locking allocator signature
// considered a 'single' allocator, so applies to usage with estd::basic_string, etc.
// i.e. the buffer managed by this NetBufAllocator applies to only *one* allocated
// entity, no further 'alloc' calls allowed
template <class T, class TNetBuf>
class NetBufAllocator
{
    TNetBuf netbuf;

public:
    // notifies support logic to treat this as stateful/an instance
    typedef void is_stateful_tag;
    typedef void is_singular_tag;
    typedef void is_locking_tag;
    typedef void has_size_tag;

    typedef const void* const_void_pointer;
    typedef typename estd::remove_reference<TNetBuf>::type netbuf_type;
    typedef bool handle_type; //single-allocator
    typedef handle_type handle_with_size;
    typedef T value_type;
    typedef T* pointer;
    typedef typename netbuf_type::size_type size_type;
    typedef estd::internal::handle_with_only_offset<handle_type, size_type> handle_with_offset;

private:
    // we're on a particular chunk, what absolute position does the beginning
    // of this chunk map to?
    size_type absolute_pos;

public:
    NetBufAllocator() : absolute_pos(0) {}

    /*
    NetBufAllocator(TNetBuf&& nb) :
        netbuf(std::move(nb)) {} */

    NetBufAllocator(TNetBuf& nb) :
        netbuf(nb),
        absolute_pos(0) {}

    /*
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ... TArgs>
    NetBufAllocator(TArgs&&...args) :
        netbuf(std::forward<TArgs>(args)...),
        absolute_pos(0) {}
#endif */

    static CONSTEXPR handle_type invalid() { return false; }

    // technically we ARE locking since we have to convert the dummy 'bool' handle
    // to a pointer
    static CONSTEXPR bool is_locking() { return true; }

    static CONSTEXPR bool is_stateful() { return true; }

    static CONSTEXPR bool is_singular() { return true; }

    // NOTE: Since we're using tags now, consider
    // tag to be is_noncontiguous since default tag state (nonexistant)
    // is false
    static CONSTEXPR bool is_contiguous() { return false; }

    static CONSTEXPR bool has_size() { return true; }

    // call not present in estd allocators. experimental
    size_type max_lock_size()
    {
        return netbuf.size();
    }

    size_type size(handle_with_size) const { return netbuf.total_size(); }

    value_type& lock(handle_type, size_type pos, size_type count)
    {
        // Netbuf will need a 'reset' to reposition chunk to the beginning
        // then, we'll need to count chunks forward until we arrive at pos.  Then,
        // we'll have to be extra sure 'count' is available.  Perhaps make going
        // past 'count' undefined, and demand caller heed max_lock_size.  Will be
        // tricky since 'pos' is from absolute start, not chunk start

        if(pos < absolute_pos)
        {
            netbuf.reset();
            absolute_pos = 0;

            while(netbuf.size() < pos)
            {
                pos -= netbuf.size();

                if(netbuf.last())
                {
                    // FIX: Just commented this out, we shouldn't be expanding
                    // inside a lock operation... that's what realloc is for
                    /*embr::mem::ExpandResult r = netbuf.expand(pos, false);

                    if(r < 0)
                    {
                        // TODO: Report error, couldn't expand to requested position
                    } */
                }

                absolute_pos += netbuf.size();
                netbuf.next();
            }
        }

        return *((char*)netbuf.data() + pos);

        // TODO: Perhaps we up any reference counter too, if NetBuf has one
    }

    void unlock(handle_type) {}
    void cunlock(handle_type) const {}

    const value_type& clock(handle_type h, size_type pos, size_type count) const
    {
        return const_cast<NetBufAllocator*>(this)->lock(h, pos, count);
    }

    handle_with_offset offset(handle_type h, size_t pos) const
    {
        return handle_with_offset(h, pos);
    }

    handle_type allocate(size_type) { return invalid(); }

    handle_with_size allocate_ext(size_type size)
    {
        // already allocated, shouldn't do it again
        return false;
    }

    handle_with_size reallocate_ext(handle_with_size, size_type size)
    {
        // already allocated, shouldn't do it again
        return false;
    }

    void deallocate(handle_type h, size_type count)
    {
    }

    handle_type reallocate(handle_type h, size_t len)
    {
        using namespace embr::mem;

        // FIX: Doublecheck and make sure this really is an expand
        len -= netbuf.total_size();

        ExpandResult r = netbuf.expand(len, true);

        if(r < 0)
        {
            // FIX: do something about a failure
        }

        /*
        // Not yet supported operation, but netbufs (usually)
        // can do this
        assert(false); */

        return h;
    }


    //typedef typename estd::nothing_allocator<T>::lock_counter lock_counter;
};


}

}

namespace estd { namespace internal { namespace impl {

#ifdef UNUSED
// NOTE: The generic one in impl/dynamic_array.h is not suitable because it doesn't (can't)
// know that we're always preallocated.  The smart-specialized one in allocators/handle_desc.h
// hopefully will participate, and would be nice if a similar notion applied to dynamic_array
// itself to auto deduce new types
template <class T, class TNetBuf, class TPolicy>
class dynamic_array<embr::experimental::NetBufAllocator<T, TNetBuf>, TPolicy > :
        public dynamic_array_base<embr::experimental::NetBufAllocator<T, TNetBuf>, false >
{
    typedef dynamic_array_base<embr::experimental::NetBufAllocator<T, TNetBuf>, false > base;

public:
#if defined(FEATURE_CPP_MOVESEMANTIC) && defined(FEATURE_CPP_VARIADIC)
    template <class ... TArgs>
    dynamic_array(TArgs&&...args) : base(std::forward<TArgs>(args)...)
    { }
#endif
};

// FIX: Very annoying to explicitly define reference version
// NOTE: Doesn't compile well, due to const inconsistencies through string/allocated_array/impl
// chains.  That needs to be ironed out
template <class T, class TNetBuf, class TPolicy>
class dynamic_array<embr::experimental::NetBufAllocator<T, TNetBuf>&, TPolicy > :
        public dynamic_array_base<embr::experimental::NetBufAllocator<T, TNetBuf>&, false >
{
    typedef dynamic_array_base<embr::experimental::NetBufAllocator<T, TNetBuf>&, false > base;

public:
#if defined(FEATURE_CPP_MOVESEMANTIC) && defined(FEATURE_CPP_VARIADIC)
    template <class ... TArgs>
    dynamic_array(TArgs&&...args) : base(std::forward<TArgs>(args)...)
    { }
#endif

    typedef typename std::remove_reference<typename base::allocator_type>::type allocator_type;
    // TODO: A lof of 'allocator_traits<TAllocator>' floating around, still need to weed that out
    typedef typename estd::allocator_traits<allocator_type> allocator_traits;
};
#endif

}}}

#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>
#include <embr/netbuf-reader.h>
#include <embr/exp/netbuf-alloc.h>

#include <estd/string.h>
#include <estd/string_view.h>

// NOTE: At this time, not yet freertos specific, so test it here in regular GNU area
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/streambuf.hpp>
#include <estd/sstream.h>

using namespace embr::experimental;

template <class TTransport, class TRetryPolicyImpl, class TTimer>
std::allocator<typename RetryManager<TTransport, TRetryPolicyImpl, TTimer>::QueuedItem>
        RetryManager<TTransport, TRetryPolicyImpl, TTimer>::stub;

template <class TAllocator>
class test_string : public estd::basic_string<
        char,
        std::char_traits<char>,
        TAllocator>
{
    typedef estd::basic_string<
            char,
            std::char_traits<char>,
            TAllocator> base;

public:
    test_string(TAllocator& a) : base(a) {}

    // NOTE: makes no difference
    typedef typename std::remove_reference<TAllocator>::type allocator_type;
};


template <class TAllocator>
void test(TAllocator& a)
{
    // FIX: Misbehaves when we really want it to be TNetBufAllocator&
    // runs destructor since we're actually copying to a local TNetBufAllocator
    // rather than a reference, which results in two active TNetBufAllocators
    // - at the moment it doesn't crash, but it will
    test_string<TAllocator&> s(a);

    s += "hello";

    REQUIRE(s == "hello");

    s += " world!";

    REQUIRE(s == "hello world!");
}

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("NetBufAllocator")
    {
        embr::mem::layer1::NetBuf<128> nb;
        NetBufAllocator<char, decltype(nb)& > a(nb);

        test(a);
    }
    SECTION("NetBufDynamic")
    {
        embr::mem::experimental::NetBufDynamic<> nb;

        REQUIRE(nb.size() == 0);
        REQUIRE(nb.data() == NULLPTR);
        REQUIRE(nb.total_size() == 0);

        SECTION("Coupled with NetBufAllocator")
        {
            // working well-ish but have yet to test actual chaining
            // it's clear naming is a little confusing here, NetBufAllocator and NetBufDynamic
            NetBufAllocator<char, decltype(nb)& > a(nb);

            test(a);
        }
    }
    SECTION("Inline NetBufAllocator + NetBufDynamic")
    {
        NetBufAllocator<char, embr::mem::experimental::NetBufDynamic<> > a;

        // is 32 bytes, likely due to some kind of padding
        //REQUIRE(sizeof(a) == sizeof(int) + sizeof(void*) + sizeof(void*));

        // this works, but destructor seems to be a bit squirrely
        test(a);
    }
    SECTION("Retry v3")
    {
        // stringbufs still a mess, so using span variety
        //typedef estd::experimental::ostringstream<128> ostream_type;
        typedef estd::experimental::ospanstream ostream_type;
        typedef estd::experimental::ispanstream istream_type;

        struct Transport
        {
            typedef int endpoint_type;

            //typedef estd::layer1::stringbuf<128> ostreambuf_type;
            typedef ostream_type::streambuf_type ostreambuf_type;
            typedef istream_type::streambuf_type istreambuf_type;
        };

        struct RetryImpl
        {
            typedef Transport transport_type;
            typedef int key_type;
            typedef unsigned timebase_type;

            struct item_policy_impl_type
            {
                timebase_type get_new_expiry()
                {
                    return 100;
                }
            };

            timebase_type get_relative_expiry(item_policy_impl_type& item)
            {
                return 100;
            }
        };


        struct TimerImpl
        {
            typedef unsigned timebase_type;
            typedef int handle_type;

            handle_type create(timebase_type expiry, void* arg)
            {
                return 0;
            }
        };

        char buf[128];
        estd::span<char> span(buf);

        ostream_type out(span);

        int fake_endpoint = 7;
        auto sb = out.rdbuf();

        embr::experimental::RetryManager<Transport, RetryImpl, TimerImpl> rm;

        // FIX: In its current state, this generates a memory leak since send does a 'new'
        rm.send(fake_endpoint, *sb, 0);
    }
}

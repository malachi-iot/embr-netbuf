#include <catch.hpp>

#include <embr/observer.h>

static int expected;

struct event_1
{
    int data;
};


struct event_2
{
    int data;
};


struct event_3
{
    int data;
};

struct noop_event {};

static int counter = 0;

class StatelessObserver
{
public:
    static void on_notify(int val)
    {
        REQUIRE(val == expected);
        counter++;
    }

    static void on_notify(event_1 val, const int& context)
    {

    }
};

class IObserver
{
public:
    virtual void on_notify(event_1 n) = 0;
};

class FakeBase {};

class StatefulObserver : public FakeBase
{
public:
    int id;

    StatefulObserver() : id(0x77) {}

    StatefulObserver(int id) : id(id) {}

    void on_notify(int val)
    {
        REQUIRE(val == expected);
    }

    static void on_notify(event_3 e, event_3& context)
    {
        REQUIRE(e.data == expected);

        context.data = 77;
    }


    void on_notify(event_1 e)
    {
        REQUIRE(e.data == expected);
    }

    void on_notify(const event_2& e)
    {
        REQUIRE(e.data == expected);
    }
};


struct OtherStatefulObserver
{
    char buf[10];

    OtherStatefulObserver()
    {
        buf[0] = 1;
        buf[1] = 2;
        buf[2] = 3;
        buf[3] = 4;
    }

    void on_notify(int val)
    {
        REQUIRE(val == expected);
    }
};


TEST_CASE("observer")
{
    SECTION("void subject")
    {
        embr::void_subject vs;

        vs.notify(3);
    }
    SECTION("next-gen tuple based observer")
    {
        counter = 0;
        expected = 3;

        SECTION("layer0")
        {
            SECTION("raw")
            {
                embr::layer0::subject<
                        StatelessObserver,
                        StatelessObserver> s;

                int sz = sizeof(s);

                s.notify(3);
                REQUIRE(counter == 2);
                s.notify(event_1 { 3 }); // goes nowhere
            }
        }
        SECTION("layer1")
        {
            embr::layer1::subject<
                    StatelessObserver,
                    StatefulObserver,
                    StatelessObserver,
                    StatelessObserver
                    > s;

            int sz = sizeof(s);

            s.notify(3);

            // count 3 times, once per stateless observer
            REQUIRE(counter == 3);
            s.notify(event_1 { 3 });

            event_3 ctx;

            // Notify is gonna evaluate whether 'data' matches 'expected', which
            // is 3 in this case
            ctx.data = expected;

            s.notify(ctx, ctx);

            // context should be modified by stateful observer

            REQUIRE(ctx.data == 77);

            SECTION("make_subject")
            {
                StatelessObserver o1;
                StatefulObserver o2(0x777);

                event_1 e;

                e.data = 0x777;

                auto s = embr::layer1::make_subject(
                            o1,
                            o2);

                // need to revise event_1 behavior or do an event_4
                //s.notify(e);
            }
            SECTION("proxy")
            {
                embr::layer1::internal::observer_proxy<decltype(s)> op(s);

                op.on_notify(noop_event{});
            }
        }
    }
}

#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-writer.h>

#include <estd/string.h>

TEST_CASE("writer test", "[writer]")
{
    SECTION("Static layer1 netbuf")
    {
        embr::mem::NetBufWriter<embr::mem::layer1::NetBuf<64>> writer;

        REQUIRE(writer.total_size() == 64);
    }
    SECTION("Static layer2 netbuf")
    {
        embr::mem::layer2::NetBuf<128> netbuf;
        embr::mem::NetBufWriter<decltype(netbuf)&> writer(netbuf);
        auto netbuf_data = reinterpret_cast<const uint8_t*>(netbuf.data());

        // NOTE: it's 0 because the vector has not been added to yet
        // a bit peculiar, but perhaps after getting use to it it'll
        // sit right
        REQUIRE(writer.buffer().size() == 0);

        SECTION("basic write")
        {
            REQUIRE(writer.next(100));
            REQUIRE(writer.total_size() >= 100); // Because of our 'room to grow' policy
            estd::mutable_buffer b = writer.buffer();

#ifdef BROKEN_AMBIGUOUS_CONSTRUCTOR
            estd::layer3::string s((char*)b.data(), 0, b.size());
            //writer.buffer();

            s += "Hi2u";
#else
            sprintf((char*)b.data(), "Hi2u");
#endif

            REQUIRE(netbuf_data[0] == 'H');
            REQUIRE(netbuf_data[1] == 'i');
            REQUIRE(netbuf_data[2] == '2');
            REQUIRE(netbuf_data[3] == 'u');
        }
        // FIX: These all actually should be failing since we didn't expand
        // but because bounds checking is currently so weak, it works
        SECTION("basic << operator")
        {
            uint8_t buffer[] = { 1, 2, 3 };

            writer << buffer;

            for(int i = 0; i < sizeof(buffer); i++)
                REQUIRE(buffer[i] == netbuf_data[i]);
        }
        SECTION("string(ish) << operator")
        {
            estd::layer2::const_string s = "hello";
            estd::const_buffer buffer((uint8_t*)s.lock(), s.size());

            writer << buffer;

            for(int i = 0; i < buffer.size(); i++)
                REQUIRE(buffer[i] == netbuf_data[i]);
        }
        SECTION("byte << operator")
        {
            uint8_t value = 0xFF;

            writer << value;

            REQUIRE(0xFF == netbuf_data[0]);
        }
        SECTION("chunked write")
        {

        }
        SECTION("experimental")
        {
            SECTION("itoa")
            {
                embr::mem::experimental::itoa(writer, 500);

                REQUIRE('5' == netbuf_data[0]);
                REQUIRE('0' == netbuf_data[1]);
                REQUIRE(0 != netbuf_data[2]);
                REQUIRE('0' == netbuf_data[2]);
            }
        }
    }
}

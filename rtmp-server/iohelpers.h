#ifndef _IO_HELPERS_H
#define _IO_HELPERS_H

#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace iohelpers
{
    int
    to_integer_big_endian(const std::string& bytes)
    {
        int res = 0;

        for (auto b : bytes)
            res = res * 0x100 + (uint8_t)b;

        return res;
    }

    int
    to_integer_little_endian(std::string bytes)
    {
        std::reverse(bytes.begin(), bytes.end());
        return to_integer_big_endian(bytes);
    }

    int
    pack_bits(const std::vector<int>& values,
              const std::vector<int>& widths)
    {
        if (values.size() != widths.size())
            throw std::runtime_error("length mismatch");

        int integer = 0;
        for (int i = 0 ; i < values.size(); ++i)
        {
            int v = values[i];
            int w = widths[i];
            if (v >= (1 << w))
                throw std::runtime_error("Some values are too large for the widths");
            integer <<= w;
            integer |= v;
        }
        return integer;
    }

    // 整数をビット幅を指定した配列にしたがって分解する。
    std::vector<int> unpack_bits(int integer, std::vector<int> sizes)
    {
        std::vector<int> res;
        for (auto size = sizes.rbegin();
             size != sizes.rend();
             ++size)
        {
            res.push_back(integer & ((1<<*size) - 1));
            integer >>= *size;
        }
        std::reverse(res.begin(), res.end());
        return res;
    }

    std::string
    to_bytes_big_endian(int integer,
                        int nbytes)
    {
        std::string res;
        for (int i = 0; i < nbytes; ++i)
        {
            res.push_back(integer & 0xff);
            integer >>= 8;
        }
        std::reverse(res.begin(), res.end());
        return res;
    }

    void test()
    {
        if (to_integer_big_endian({ 0x01, 0x00 }) != 256)
            abort();
        if (to_integer_little_endian({ 0x00, 0x01 }) != 256)
            abort();
        if (pack_bits({1, 2, 3, 4}, {4, 4, 4, 4}) != 0x1234)
            abort();

        {
            auto s = to_bytes_big_endian(0x1234, 4);
            if (s.size() != 4 ||
                s[0] != 0 ||
                s[1] != 0 ||
                s[2] != 0x12 ||
                s[3] != 0x34)
                abort();
        }

        {
            // unpack_bits(0b11000001, [2, 6]) = [3, 1] = [0b11, 0b000001]
            // 1100 0001 = 0xC1
            auto v = unpack_bits(0xC1, {2, 6});
            if (v.size() != 2 ||
                v[0] != 3 ||
                v[1] != 1)
                abort();
        }
    }
}

#endif

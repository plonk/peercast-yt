#include "iohelpers.h"
#include <assert.h>

namespace rtmpserver
{
    using namespace iohelpers;

    struct Message
    {
        int32_t timestamp;
        int length;
        int type_id;
        int stream_id;
        std::string data;

        // マップで値として使うためのデフォルトコンストラクタ。
        Message()
            : timestamp ( 0 )
            , length ( 0 )
            , type_id ( 0 )
            , stream_id ( 0 )
        {}

        Message(int32_t aTimestamp, int aLength, int aType_id, int aStream_Id)
            : timestamp ( aTimestamp )
            , length ( aLength )
            , type_id ( aType_id )
            , stream_id ( aStream_Id )
        {}

        void
        add_data(const std::string& chunk_data)
        {
            data += chunk_data;
        }

        // あと何バイト追加したらメッセージが完成するか。名前がまぎら
        // わしいな。正の量に聴こえる。
        int
        remaining()
        {
            assert(length - data.size() >= 0);
            return length - data.size();
        }

        std::vector<std::string>
        to_chunks(int maximum_chunk_size,
                  int chunk_stream_id)
        {
            assert(this->remaining() == 0);

            std::vector<std::string> chunks;

            int remaining = data.size();

            while (remaining != 0)
            {
                int chunk_size = std::min(remaining, maximum_chunk_size); // size of this chunk
                std::string header;
                if (chunk_size == data.size())
                {
                    header =
                        make_basic_header(0, chunk_stream_id) +
                        make_type0_header(0, data.size(), type_id, stream_id);
                }else
                {
                    header = make_basic_header(3, chunk_stream_id);
                }

                auto chunk_data = data.substr(data.size() - remaining, chunk_size);
                chunks.push_back(header + chunk_data);
                remaining -= chunk_size;
            }
            return chunks;
        }

        std::string make_basic_header(int fmt, int cs_id)
        {
            int byte = pack_bits({fmt, cs_id}, {2, 6});
            return std::string({ (char)byte });
        }

        std::string make_type0_header(int32_t timestamp,
                                      int length,
                                      int type,
                                      int stream_id)
        {
            if (timestamp >= 0xffffff)
                throw std::runtime_error("extended timestamp not implemented");

            std::string t = to_bytes_big_endian(timestamp, 3);
            std::string l = to_bytes_big_endian(length, 3);
            std::string s = to_bytes_little_endian(stream_id, 4);
            return t + l + std::string({ (char)type }) + s;
        }
    };
}

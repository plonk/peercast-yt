#include <stdio.h>
#include <map>
#include <tuple>
#include "message.h"
#include "amf0.h"
#include "sstream.h"
#include "flvwriter.h"

namespace rtmpserver
{
    using namespace iohelpers;
    using namespace amf0;

    class Session
    {
    public:
        ClientSocket* client;
        FLVWriter& flv_writer;

        int max_incoming_chunk_size;
        int max_outgoing_chunk_size;

        bool quitting;

        Session(ClientSocket* aClient, FLVWriter& aFlvWriter)
            : client(aClient)
            , max_incoming_chunk_size(128)
            , max_outgoing_chunk_size(128)
            , flv_writer(aFlvWriter)
            , quitting(false) {}

        // 毎回同じ乱数列を生成する。
        static std::string generate_random_bytes(int size = 1528)
        {
            std::string res;

            srand(0);
            for (int i = 0; i < 1528; ++i)
                res.push_back(rand() % 256);

            return res;
        }

        std::tuple<int,int,int,int> read_type0_header(Stream* io)
        {
            // int でいいの？ wrap-round 大丈夫？
            int  timestamp         = to_integer_big_endian({ io->readChar(), io->readChar(), io->readChar() });
            int  message_length    = to_integer_big_endian({ io->readChar(), io->readChar(), io->readChar() });
            char message_type_id   = io->readChar();
            int  message_stream_id = to_integer_little_endian({ io->readChar(), io->readChar(), io->readChar(), io->readChar() });
            if (timestamp >= 0xffffff)
                throw std::runtime_error("extended timestamp not implemented");
            return std::make_tuple(timestamp, message_length, message_type_id, message_stream_id);
        }

        std::tuple<int,int,int> read_type1_header(Stream* io)
        {
            int tdelta = to_integer_big_endian({ io->readChar(), io->readChar(), io->readChar() });
            int length = to_integer_big_endian({ io->readChar(), io->readChar(), io->readChar() });
            int type = io->readChar();

            if (tdelta >= 0xffffff)
                throw std::runtime_error("extended timestamp not implemented");
            return std::make_tuple(tdelta, length, type);
        }

        void run()
        {
            handshake(client);

            // 各チャンクストリームの最後のメッセージを記録する map。
            std::map<int,Message> cstreams;

            while (!quitting)
            {
                uint8_t basic_header = client->readChar();
                auto v = unpack_bits(basic_header, {2, 6});
                int fmt = v[0];
                int cs_id = v[1];

                //printf("fmt = %d, cs_id = %d\n", fmt, cs_id);

                if ( !(cs_id >= 2 && cs_id < 64) )
                    throw std::runtime_error("cs_id out of range");

                switch (fmt)
                {
                case 0: {
                    int timestamp, length, type, stream;
                    std::tie(timestamp, length, type, stream) =
                        read_type0_header(client);

                    cstreams[cs_id] = Message(timestamp, length, type, stream);
                    break;
                }
                case 1: {
                    if (!cstreams.count(cs_id))
                        throw std::runtime_error("protocol error");
                    Message prev = cstreams[cs_id];
                    int tdelta, length, type;
                    std::tie(tdelta, length, type) = read_type1_header(client);
                    cstreams[cs_id] = Message(prev.timestamp + tdelta,
                                              length,
                                              type,
                                              prev.stream_id);
                    break;
                }
                case 2: {
                    if (!cstreams.count(cs_id))
                        throw std::runtime_error("protocol error");

                    Message prev = cstreams[cs_id];
                    assert(prev.remaining() == 0);

                    int tdelta = to_integer_big_endian(client->Stream::read(3));
                    cstreams[cs_id] = Message(prev.timestamp + tdelta, prev.length, prev.type_id, prev.stream_id);
                    break;
                }
                case 3: {
                    if (!cstreams.count(cs_id))
                        throw std::runtime_error("protocol error");
                    Message prev = cstreams[cs_id];
                    if (prev.remaining() == 0)
                    {
                        // previous message is complete.
                        // this chunk begins a new message.
                        cstreams[cs_id] = Message(prev.timestamp, prev.length, prev.type_id, prev.stream_id);
                    }
                    break;
                }
                default: throw std::logic_error("invalid fmt");
                }

                Message& message = cstreams[cs_id];
                int chunk_size = std::min(message.remaining(),
                                          max_incoming_chunk_size);
                message.add_data(client->Stream::read(chunk_size));

                if (message.remaining() == 0)
                {
                    //puts("message complete");
                    on_message(message);
                }
            } // while (true)
        }

        void on_connect(Value& transaction_id, const std::vector<Value>& params)
        {
            //puts("sending ServBW");
            { // send ServerBW
                Message message(0, 4, 0x05, 0); // timestamp, len, type, msg. stream id
                message.add_data(to_bytes_big_endian(5000*1000, 4));
                std::string chunks = str::join("", message.to_chunks(max_outgoing_chunk_size, 2));
                client->writeString(chunks);
            }

            //puts("sending ClientBW");
            { // send ClientBW
                Message message(0, 5, 0x06, 0);
                message.add_data(to_bytes_big_endian(5000*1000, 4) + "\x01");  // 0: Hard, 1: Soft, 2: Dynamic
                client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 2)));
            }

            //puts("sending SetPacketSize");
            { // send SetPacketSize
                Message message(0, 4, 0x01, 0);
                message.add_data(to_bytes_big_endian(4096, 4));
                client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 2)));
                max_outgoing_chunk_size = 4096;
            }

            //puts("sending _result");
            { // send command result
                std::string data;

                data += Value("_result").serialize();
                data += transaction_id.serialize();
                data += Value::object({ {"fmsVer","FMS/3,0,1,123"}, {"capabilities",31} }).serialize();
                data += Value::object(
                    {
                        {"level","status"},
                        {"code","NetConnection.Connect.Success"},
                        {"description","Connection succeeded."},
                        {"objectEncoding",0}
                    }).serialize();

                Message message(0, data.size(), 0x14, 0);
                message.add_data(data);
                client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 3)));
            }

            flv_writer.writeFileHeader();
        }

        void on_FCPublish(Value& transaction_id, const std::vector<Value>& params)
        {
            std::string data;

            data += Value("_result").serialize();
            data += transaction_id.serialize();
            data += params[1].serialize(); // name
            Message message(0, data.size(), 0x14, 0);
            message.add_data(data);
            client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 3)));
        }

        void on_createStream(Value& transaction_id, const std::vector<Value>& params)
        {
            std::string data;
            data += Value("_result").serialize();
            data += transaction_id.serialize();
            data += Value(nullptr).serialize();
            data += Value(1).serialize();
            Message message(0, data.size(), 0x14, 0);
            message.add_data(data);
            client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 3)));
        }

        void on_publish(Value& transaction_id, const std::vector<Value>& params)
        {
            //puts("Sending onStatus in response to publish command");
            std::string data;
            data += Value("onStatus").serialize();
            data += Value(0).serialize();
            data += Value(nullptr).serialize();
            data += Value::object(
                {
                    { "level", "status" },
                    { "code", "NetStream.Publish.Start" },
                    { "description", "Start publishing" }
                }).serialize();
            Message message(0, data.size(), 0x14, 1);
            message.add_data(data);
            client->writeString(str::join("", message.to_chunks(max_outgoing_chunk_size, 8)));
        }

        void on_deleteStream(Value& transaction_id, const std::vector<Value>& params)
        {
            quitting = true;
        }

        void on_command(Message& message)
        {
            // メッセージから command, transaction_id, params を抽出。
            StringStream mem(message.data);
            amf0::Deserializer d;
            Value command = d.readValue(mem);
            printf("command = %s\n", command.inspect().c_str());
            Value transaction_id = d.readValue(mem);
            printf("transaction_id = %s\n", transaction_id.inspect().c_str());
            std::vector<Value> params;
            for (int i = 0; i < 2 && !mem.eof(); i++)
            {
                Value param = d.readValue(mem);
                printf("param%d = %s\n", (i+1), param.inspect().c_str());
                params.push_back(param);
            }

            // ディスパッチ。
            auto c = command.string();
            if (c == "connect")
                on_connect(transaction_id, params);
            else if (c == "FCPublish")
                on_FCPublish(transaction_id, params);
            else if (c == "createStream")
                on_createStream(transaction_id, params);
            else if (c == "publish")
                on_publish(transaction_id, params);
            else if (c == "deleteStream")
                on_deleteStream(transaction_id, params);
            else
                printf("ignoring command %s\n", command.string().c_str());
        }

        void on_metadata(Message& message)
        {
            puts("Got metadata");

            StringStream mem(message.data);
            amf0::Deserializer d;
            Value v = d.readValue(mem);
            printf("%s\n", v.inspect().c_str());

            int pos = mem.getPosition();
            flv_writer.writeScriptTag(message.timestamp, message.data.substr(pos));
        }

        void on_set_chunk_size(Message& message)
        {
            int new_chunk_size = to_integer_big_endian(message.data.substr(0,4));
            printf("max incoming chunk size is now %d (was %d)\n",
                   new_chunk_size,
                   max_incoming_chunk_size);
            max_incoming_chunk_size = new_chunk_size;
        }

        void on_audio_packet(Message& message)
        {
            flv_writer.writeAudioTag(message.timestamp, message.data);
        }

        void on_video_packet(Message& message)
        {
            flv_writer.writeVideoTag(message.timestamp, message.data);
        }

        void on_message(Message& message)
        {
            assert(message.remaining() == 0);

            switch (message.type_id)
            {
            case 0x14: // Command
                on_command(message);
                break;
            case 0x12:
                on_metadata(message);
                break;
            case 0x01:
                on_set_chunk_size(message);
                break;
            case 0x08:
                on_audio_packet(message);
                break;
            case 0x09:
                on_video_packet(message);
                break;
            default:
                printf("unknown message type id %d", message.type_id);
            }
        }

        void handshake(ClientSocket* client)
        {
            // receive C0
            char c0 = client->readChar();
            if (c0 == 3)
            {
                //puts("client requests version 3");
            }else if (c0 >= 32)
            {
                throw std::runtime_error("invalid C0");
            }

            // send S0 + S1
            client->writeChar(3);

            std::string s1 =
                to_bytes_big_endian(0x0, 4) +
                to_bytes_big_endian(0x0d0e0a0d, 4) +
                generate_random_bytes();
            client->writeString(s1);

            // receive C1
            std::string c1 = client->Stream::read(1536);
            auto time = c1.substr(0, 4);
            auto zero = c1.substr(4, 4);
            assert(zero.size() == 4);
            std::string random_bytes = c1.substr(8, 1528);

            // send S2
            std::string s2 = time + std::string({0,0,0,0}) + random_bytes;
            client->writeString(s2);

            // receive C2
            std::string c2 = client->Stream::read(1536);
            // 解析は省略。
        }

        static void test()
        {
            {
                auto bytes = generate_random_bytes();
                assert(bytes.size() == 1528);

                assert(generate_random_bytes() == bytes);
            }
        }

    }; // class Session

} // namespace rtmpserver

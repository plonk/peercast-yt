namespace rtmpserver
{
    struct FLVWriter
    {
        static const int STREAM_ID = 0;

        // tag types
        static const int TT_AUDIO = 8;
        static const int TT_VIDEO = 9;
        static const int TT_SCRIPT = 18;

        enum State
        {
            kExpectFileHeader,
            kExpectScriptTag,
            kExpectDataTag,
        };

        Stream& out;

        State state;
        int previous_tag_size;

        FLVWriter(Stream& io)
            : out(io)
            , state(kExpectFileHeader)
            , previous_tag_size(0) {}

        ~FLVWriter()
        {
            out.close();
        }

        void writeFileHeader(bool audio = true, bool video = true)
        {
            if (state != kExpectFileHeader)
                throw std::runtime_error("invalid operation");

            int type_flags = pack_bits({0, audio, 0, video}, {5, 1, 1, 1});
            std::string data("FLV");
            data += 1;
            data += type_flags;
            data += to_bytes_big_endian(9, 4);
            assert(data.size() == 9);
            out.writeString(data);
            state = kExpectScriptTag;
        }

        void writeScriptTag(int32_t timestamp, const std::string& data)
        {
            if (state != kExpectScriptTag)
                throw std::runtime_error("invalid operation");

            int32_t timestamp1;
            uint8_t timestamp_extended;
            std::tie(timestamp1, timestamp_extended) = splitTimestamp(timestamp);
            writeTag(TT_SCRIPT, timestamp1, timestamp_extended, data);

            state = kExpectDataTag;
        }

        void writeVideoTag(int32_t timestamp, const std::string& data)
        {
            if (state != kExpectDataTag)
                throw std::runtime_error("invalid operation");

            int32_t timestamp1;
            uint8_t timestamp_extended;
            std::tie(timestamp1, timestamp_extended) = splitTimestamp(timestamp);
            writeTag(TT_VIDEO, timestamp1, timestamp_extended, data);
        }

        void writeAudioTag(int32_t timestamp, const std::string& data)
        {
            if (state != kExpectDataTag)
                throw std::runtime_error("invalid operation");

            int32_t timestamp1;
            uint8_t timestamp_extended;
            std::tie(timestamp1, timestamp_extended) = splitTimestamp(timestamp);
            writeTag(TT_AUDIO, timestamp1, timestamp_extended, data);
        }

        // XXX: Unimplemented
        // conversion of a SI32 timestamp to UI24 + UI8.
        std::pair<int32_t,uint8_t>
        splitTimestamp(int32_t timestamp)
        {
            return {timestamp, 0};
        }

        void writeTag(int tag_type,
                      int timestamp,
                      char timestamp_extended, 
                      const std::string& data)
        {
            // write previous tag size
            out.writeString(to_bytes_big_endian(previous_tag_size, 4));

            // write tag header
            out.writeString(
                std::string({(char)tag_type}) +
                to_bytes_big_endian(data.size(), 3) +
                to_bytes_big_endian(timestamp, 3) +
                std::string({timestamp_extended}) +
                to_bytes_big_endian(STREAM_ID, 3));

            // write payload
            out.writeString(data);

            previous_tag_size = data.size() + 11;
        }

    };
} // namespace rtmpserver

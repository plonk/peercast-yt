#ifndef _DMSTREAM_H
#define _DMSTREAM_H

#include <vector>
#include "stream.h"
#include <string>

class StringStream : public Stream
{
public:
    StringStream();
    StringStream(const std::string&);

    int  read(void *, int) override;
    void write(const void *, int) override;
    bool eof() override;
    void rewind() override;
    void seekTo(int) override;
    int  getPosition() override;
    int  getLength();

    std::string str();
    void str(const std::string& data);

    void checkSize(size_t);

    size_t m_pos;
    std::string m_buffer;
};

#endif

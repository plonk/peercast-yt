#include "wmhttp.h"

extern ASFInfo parseASFHeader(Stream &in); // from mms.cpp

void WMHTTPStream::readEnd(Stream &, Channel *)
{
}

// ------------------------------------------
void WMHTTPStream::readHeader(Stream &, Channel *)
{
}

// ------------------------------------------
int WMHTTPStream::readPacket(Stream &in, Channel *ch)
{
    WMHTTPChunk c;

    c.read(in);

    if (c.type == "$E") // End of stream
    {
        return 1;
    }
    if (c.type != "$H" && c.type != "$D")
        return 0;

    ASFChunk chunk = c.getASFChunk(m_seqno++);

    MMSStream::processChunk(in, ch, chunk);
    return 0;
}

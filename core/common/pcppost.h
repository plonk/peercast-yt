class PCPPostSource : public ChannelSource
{
    void stream(Channel *ch) override
    {
        //while (ch->thread.active)
        {
            ch->setStatus(Channel::S_BROADCASTING);
            ch->sourceStream = ch->createSource();
            ch->readStream(*ch->sock, ch->sourceStream);
        }
    }
};

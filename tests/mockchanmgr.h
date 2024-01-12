#ifndef MOCKCHANMGR_H
#define MOCKCHANMGR_H

#include "chanmgr.h"

class MockChanMgr : public ChanMgr
{
public:
    MockChanMgr() {}

    std::shared_ptr<ChanHit> addHit(ChanHit &h) override
    {
        lastHitAdded = h;
        return ChanMgr::addHit(h);
    }

    ChanHit lastHitAdded;

};


#endif

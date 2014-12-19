#ifndef ALLOC_H
#define ALLOC_H

#include "base_alloc.h"

template <TVirtPtrSize POOL_SIZE, TVirtPtrSize PAGE_SIZE, uint8_t PAGE_COUNT>
class CVirtMemAlloc : public CBaseVirtMemAlloc
{
    SMemPage memPages[PAGE_COUNT];
    uint8_t memPagePools[PAGE_COUNT][PAGE_SIZE];

protected:
      CVirtMemAlloc(void) : CBaseVirtMemAlloc(memPages, PAGE_COUNT, POOL_SIZE, PAGE_SIZE)

    {
        for (uint8_t i=0; i<PAGE_COUNT; ++i)
            memPages[i].pool = &memPagePools[i][0];
    }
};

#endif // ALLOC_H
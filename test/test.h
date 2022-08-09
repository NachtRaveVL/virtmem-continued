#ifndef TEST_H
#define TEST_H

#include "gtest/gtest.h"

#include <inttypes.h>

using namespace virtmem;

typedef StdioVAlloc::TVPtr<uint8_t>::type UCharVirtPtr;
typedef StdioVAlloc::TVPtr<char>::type CharVirtPtr;

class VAllocFixture: public ::testing::Test
{
protected:
    StdioVAlloc vAlloc;

public:
    void SetUp(void) { vAlloc.setPoolSize(1024 * 1024 * 10); vAlloc.start(); }
    void TearDown(void) { vAlloc.stop(); }
};

template <typename T> class VPtrFixture: public VAllocFixture
{
protected:
    typename StdioVAlloc::TVPtr<T>::type vptr;

public:
    VPtrFixture(void) : vptr() { } // UNDONE: we need this for proper construction, problem?
};

#if 0
// From http://stackoverflow.com/a/17236988
inline void print128int(__uint128_t x)
{
    printf("__int128: %016" PRIx64 "%016" PRIx64 "\n",(uint64_t)(x>>64),(uint64_t)x);
}
#endif

#endif // TEST_H

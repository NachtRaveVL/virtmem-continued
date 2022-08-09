#include "virtmem-continued.h"
#include "alloc/stdio_alloc.h"
#include "test.h"

#include <vector>


TEST_F(VAllocFixture, SimpleAllocTest)
{
    const VPtrNum ptr = vAlloc.allocRaw(sizeof(int));
    ASSERT_NE(ptr, 0);

    int val = 55;
    vAlloc.write(ptr, &val, sizeof(val));
    EXPECT_EQ(*(int *)vAlloc.read(ptr, sizeof(val)), val);
    vAlloc.flush();
    EXPECT_EQ(*(int *)vAlloc.read(ptr, sizeof(val)), val);
    vAlloc.clearPages();
    EXPECT_EQ(*(int *)vAlloc.read(ptr, sizeof(val)), val);

    vAlloc.freeRaw(ptr);
}

TEST_F(VAllocFixture, ReadOnlyTest)
{
    const VPtrNum ptr = vAlloc.allocRaw(sizeof(int));
    int val = 55;
    vAlloc.write(ptr, &val, sizeof(val));
    vAlloc.flush();

    // Read only read, shouldn't modify actual memory
    int *pval = (int *)vAlloc.read(ptr, sizeof(val));
    *pval = 66;
    vAlloc.flush();
    EXPECT_EQ(*(int *)vAlloc.read(ptr, sizeof(val)), 66);
    vAlloc.clearPages();
    EXPECT_EQ(*(int *)vAlloc.read(ptr, sizeof(val)), val);
}

TEST_F(VAllocFixture, MultiAllocTest)
{
    std::vector<VPtrNum> ptrlist;
    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        ptrlist.push_back(vAlloc.allocRaw(vAlloc.getBigPageSize()));
        vAlloc.write(ptrlist[i], &i, sizeof(int));
        EXPECT_EQ(*(int *)vAlloc.read(ptrlist[i], sizeof(int)), i);
    }

    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        EXPECT_EQ(*(int *)vAlloc.read(ptrlist[i], sizeof(int)), i);
    }

    vAlloc.clearPages();

    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        EXPECT_EQ(*(int *)vAlloc.read(ptrlist[i], sizeof(int)), i);
    }
}

TEST_F(VAllocFixture, SimplePageTest)
{
    EXPECT_EQ(vAlloc.getFreeBigPages(), vAlloc.getBigPageCount());

    std::vector<VPtrNum> ptrlist;
    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        ptrlist.push_back(vAlloc.allocRaw(vAlloc.getBigPageSize()));
        vAlloc.write(ptrlist[i], &i, sizeof(int));
    }

    EXPECT_EQ(vAlloc.getFreeBigPages(), 0);

    vAlloc.flush();

    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        EXPECT_EQ(*(int *)vAlloc.read(ptrlist[i], sizeof(int)), i);
    }

    vAlloc.clearPages();
    EXPECT_EQ(vAlloc.getFreeBigPages(), vAlloc.getBigPageCount());

    for (int i=0; i<(int)vAlloc.getBigPageCount(); ++i)
    {
        EXPECT_EQ(*(int *)vAlloc.read(ptrlist[i], sizeof(int)), i);
    }
}

TEST_F(VAllocFixture, PageLockTest)
{
    EXPECT_EQ(vAlloc.getUnlockedSmallPages(), vAlloc.getSmallPageCount());
    EXPECT_EQ(vAlloc.getUnlockedMediumPages(), vAlloc.getMediumPageCount());
    EXPECT_EQ(vAlloc.getUnlockedBigPages(), vAlloc.getBigPageCount());

    // 10 is an arbitrary number, just make sure that numbers are unique, don't start at the beginning
    // and don't overlap
    auto genptr = [this](uint8_t p) { return p * vAlloc.getBigPageSize() + 10; };

    // Lock all big pages
    for (uint8_t p=0; p<vAlloc.getBigPageCount(); ++p)
    {
        vAlloc.makeDataLock(genptr(p), vAlloc.getBigPageSize());
        EXPECT_EQ(vAlloc.getUnlockedBigPages(), (vAlloc.getBigPageCount() - (p+1)));
    }

    EXPECT_EQ(vAlloc.getUnlockedBigPages(), 0);

    // lock smaller pages, shouldn't depend on big pages (anymore)
    vAlloc.makeDataLock(genptr(vAlloc.getBigPageCount() + 1), vAlloc.getSmallPageSize());
    EXPECT_EQ(vAlloc.getUnlockedSmallPages(), (vAlloc.getSmallPageCount()-1));
    vAlloc.makeDataLock(genptr(vAlloc.getBigPageCount() + 2), vAlloc.getMediumPageSize());
    EXPECT_EQ(vAlloc.getUnlockedMediumPages(), (vAlloc.getMediumPageCount()-1));


    // Unlock all big pages
    for (uint8_t p=0; p<vAlloc.getBigPageCount(); ++p)
    {
        vAlloc.releaseLock(genptr(p));
        EXPECT_EQ(vAlloc.getUnlockedBigPages(), (p+1));
    }

    vAlloc.releaseLock(genptr(vAlloc.getBigPageCount() + 1));
    EXPECT_EQ(vAlloc.getUnlockedSmallPages(), vAlloc.getSmallPageCount());
    vAlloc.releaseLock(genptr(vAlloc.getBigPageCount() + 2));
    EXPECT_EQ(vAlloc.getUnlockedMediumPages(), vAlloc.getMediumPageCount());
}

TEST_F(VAllocFixture, LargeDataTest)
{
    const VPtrSize size = 1024 * 1024 * 8; // 8 mb data block

    const VPtrNum vbuffer = vAlloc.allocRaw(size);
    for (VPtrSize i=0; i<size; ++i)
    {
        char val = size - i;
        vAlloc.write(vbuffer + i, &val, sizeof(val));
    }

    vAlloc.clearPages();

    // Linear access check
    for (VPtrSize i=0; i<size; ++i)
    {
        char val = size - i;
        ASSERT_EQ(*(char *)vAlloc.read(vbuffer + i, sizeof(val)), val);
    }

    vAlloc.clearPages();

    // Random access check
    for (VPtrSize i=0; i<200; ++i)
    {
        const VPtrSize index = (rand() % size);
        char val = size - index;
        ASSERT_EQ(*(char *)vAlloc.read(vbuffer + index, sizeof(val)), val);
    }
}

TEST_F(VAllocFixture, LargeRandomDataTest)
{
    const VPtrSize size = 1024 * 1024 * 8; // 8 mb data block

    srand(::testing::UnitTest::GetInstance()->random_seed());
    std::vector<char> buffer;
    buffer.reserve(size);
    for (size_t s=0; s<size; ++s)
        buffer.push_back(rand());

    const VPtrNum vbuffer = vAlloc.allocRaw(size);
    for (VPtrSize i=0; i<size; ++i)
    {
        char val = buffer[i];
        vAlloc.write(vbuffer + i, &val, sizeof(val));
    }

    vAlloc.clearPages();

    // Linear access check
    for (VPtrSize i=0; i<size; ++i)
    {
        ASSERT_EQ(*(char *)vAlloc.read(vbuffer + i, sizeof(char)), buffer[i]);
    }

    vAlloc.clearPages();

    // Random access check
    for (VPtrSize i=0; i<200; ++i)
    {
        const VPtrSize index = (rand() % size);
        ASSERT_EQ(*(char *)vAlloc.read(vbuffer + index, sizeof(char)), buffer[index]);
    }
}


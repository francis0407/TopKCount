#include <stdio.h>

#include "gtest/gtest.h"
#include "LRUCache.h"


class LRUCacheTest : public testing::Test {
public:
    virtual void TearDown() override {
        
    }

    void cleanFiles(std::string filePrefix, int num) {
        for (int i = 0; i < num; i++)  {
            std::string fileName = filePrefix + std::to_string(i);
            remove(fileName.data());    
        }
    }
};

TEST_F(LRUCacheTest, BlockExchange) {
    LRUCache *cache = new LRUCache(2, 3, "CacheTestFile");
    URLBlock* block = (URLBlock*)cache->getBlock(0, 0);
    block->tryAppendURL(std::string("aaaaaa"));
    block = (URLBlock*)cache->getBlock(0, 1);
    block->tryAppendURL(std::string("bbbbbb"));
    block = (URLBlock*)cache->getBlock(1, 0);
    block->tryAppendURL(std::string("cccccc"));
    block = (URLBlock*)cache->getBlock(1, 1);
    block->tryAppendURL(std::string("dddddd"));
    
    block = (URLBlock*)cache->getBlock(0, 0);
    ASSERT_EQ(block->findURL(std::string("aaaaaa")), 0);
    block = (URLBlock*)cache->getBlock(0, 1);
    ASSERT_EQ(block->findURL(std::string("bbbbbb")), 0);
    block = (URLBlock*)cache->getBlock(1, 0);
    ASSERT_EQ(block->findURL(std::string("cccccc")), 0);
    block = (URLBlock*)cache->getBlock(1, 1);
    ASSERT_EQ(block->findURL(std::string("dddddd")), 0);
    delete cache;
    cleanFiles("CacheTestFile", 2);
}

TEST_F(LRUCacheTest, CachedGroup) {
    LRUCache *cache = new LRUCache(2, 3, "CacheTestFile");
    URLBlock* block = (URLBlock*)cache->getBlock(0, 0);
    block->tryAppendURL(std::string("aaaaaa"));
    block = (URLBlock*)cache->getBlock(0, 1);
    block->tryAppendURL(std::string("bbbbbb"));
    block = (URLBlock*)cache->getBlock(0, 0);
    block->tryAppendURL(std::string("cccccc"));
    block = (URLBlock*)cache->getBlock(0, 1);
    block->tryAppendURL(std::string("dddddd"));
    auto group0 = cache->getCachedGroup(0);
    ASSERT_EQ(group0.size(), 2);
    auto group1 = cache->getCachedGroup(1);
    ASSERT_EQ(group1.size(), 1); 

    block = (URLBlock*)cache->getBlock(0, 2);
    block->tryAppendURL(std::string("xxxxxx"));
    auto group0_1 = cache->getCachedGroup(0);
    ASSERT_EQ(group0_1.size(), 3);
    auto group1_1 = cache->getCachedGroup(1);
    ASSERT_EQ(group1_1.size(), 0);

    delete cache;
    cleanFiles("CacheTestFile", 2);
}
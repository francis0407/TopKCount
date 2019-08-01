#include "gtest/gtest.h"
#include "Block.h"

class BlockTest : public testing::Test {

};

TEST_F(BlockTest, URLBlocks) {
    char* data = new char[BLOCKSIZE];
    URLBlock *block = (URLBlock*) new Block(data);
    block->reset();
    std::string url;

    url.assign(10, 'x');
    ASSERT_TRUE(block->tryAppendURL(url));
    url.assign(20, 'y');
    ASSERT_TRUE(block->tryAppendURL(url));
    url.assign(1000, 'z');
    ASSERT_TRUE(block->tryAppendURL(url));

    url.assign(4000, 'f');
    ASSERT_FALSE(block->tryAppendURL(url));

    ASSERT_EQ(block->findURL("1234"), -1);
    ASSERT_EQ(block->findURL(std::string(10,'x')), 0);
    ASSERT_EQ(block->findURL(std::string(1000, 'z')), 2);
    
    auto l = block->getURLs();
    ASSERT_EQ(l.size(), 3);
    ASSERT_EQ(l.front(), std::string(10, 'x')); 
    l.pop_front();
    ASSERT_EQ(l.front(), std::string(20, 'y'));
    l.pop_front();
    ASSERT_EQ(l.front(), std::string(1000, 'z'));

    delete block;
    delete data;
}

TEST_F(BlockTest, CountBlocks) {
    char* data = new char[BLOCKSIZE];
    CountBlock *block = (CountBlock*) new Block(data);
    block->reset();
    block->addOne(0);
    block->addOne(0);
    block->addOne(1);
    block->addOne(2);
    block->addOne(1);
    block->addOne(1);
    auto l = block->getCounts();
    ASSERT_EQ(l.size(), 3);
    ASSERT_EQ(l.front(), 2);
    l.pop_front();
    ASSERT_EQ(l.front(), 3);
    l.pop_front();
    ASSERT_EQ(l.front(), 1);
    delete block;
    delete data;
}
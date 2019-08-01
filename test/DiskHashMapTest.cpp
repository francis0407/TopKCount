#include "gtest/gtest.h"
#include "DiskURLHashMap.h"

class DiskHashMapTest : public testing::Test {
public:
    void cleanFiles(std::string filePrefix, int num) {
        for (int i = 0; i < num; i++)  {
            std::string fileName = filePrefix + std::to_string(i);
            remove(fileName.data());    
        }
    }
};

TEST_F(DiskHashMapTest, DiskHashMapTest) {
    DiskURLHashMap *map = new DiskURLHashMap(2);
    for (int i = 0; i < 10; i++)
        map->addOne(std::string(1000, 'a' + i));
    for (int i = 0; i < 5; i++)
        map->addOne(std::string(1000, 'a' + i));
    for (int i = 0; i < 3; i++)
        map->addOne(std::string(1000, 'a' + i));
    map->prepareForRead();
    std::string url;
    long long count;
    std::map<std::string, int> answer;
    for (int i = 0; i < 10; i++) {
        if (i < 3) answer[std::string(1000, 'a' + i)] = 3;
        else if (i < 5) answer[std::string(1000, 'a' + i)] = 2;
        else answer[std::string(1000, 'a' + i)] = 1;
    }
    int url_count = 0;
    while (map->readNext(&url, &count)) {
        ASSERT_EQ(answer[url], count);
        url_count ++;
    }
    ASSERT_EQ(url_count, answer.size());
    delete map;
    cleanFiles("count", 2);
    cleanFiles("url", 2);
}
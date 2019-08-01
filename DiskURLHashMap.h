#ifndef DISKURLHASHMAP_H
#define DISKURLHASHMAP_H

#include <string>
#include <unordered_set>
#include <vector>
#include <list>

#include "LRUCache.h"
#include "Block.h"

#define URLCACHESIZE (640 * 1024 * 1024)
#define COUNTCACHESIZE (128 * 1024 * 1024)

class DiskURLHashMap {
public:
    DiskURLHashMap(const int &_groups) {
        groups = _groups;
        urlOffsets.resize(groups);
        urlNums.resize(groups);
        for (int i = 0; i < groups; i++)
            urlOffsets[i].push_back(0);
        urlCache = new LRUCache(groups, URLCACHESIZE / BLOCKSIZE, "url");
        countCache = new LRUCache(groups, COUNTCACHESIZE / BLOCKSIZE, "count");
        currentURLBlock = currentCountBlock = currentGroup = 0;
    }

    ~DiskURLHashMap() {
        delete urlCache;
        delete countCache;
    }

    void addOne(const std::string &url) {
        unsigned int group = urlHash(url) % groups;
        // 1. check cached blocks (this works for frequent urls)
        auto cachedURLBlocks = urlCache->getCachedGroup(group);
        std::unordered_set<int> checkedBlocks;
        for (auto bn: cachedURLBlocks) {
            URLBlock* block = (URLBlock*)bn.first;
            int blockNum = bn.second;
            int offset = block->findURL(url);
            if (offset != -1) {
                // find in cache, update the countBlock
                offset = offset + urlOffsets[group][blockNum];
                // refresh LRU cache for the target Block
                urlCache->getBlock(group, blockNum);
                updateCountCache(group, offset);
                return;
            }
            checkedBlocks.insert(blockNum);
        }
        // 2. not find in cache, check other blocks
        for (int blockNum = 0; blockNum < urlOffsets[group].size(); blockNum++) {
            if (checkedBlocks.find(blockNum) != checkedBlocks.end())
                continue;
            // haven't been checked
            URLBlock* block = (URLBlock*)urlCache->getBlock(group, blockNum);
            int offset = block->findURL(url);
            if (offset != -1) {
                // url already exists
                offset = offset + urlOffsets[group][blockNum];
                updateCountCache(group, offset);
                return;
            }
        }
        // 3. url doesn't exist, append new url
        appendNewURL(group, url);
    }   

    void prepareForRead() {
        readURLs(0, 0);
        readCounts(0, 0);
    }
    
    bool readNext(std::string *url, long long int *count) {
        if (currentGroup >= groups)
            return false;
        if (currentURLBlock >= urlOffsets[currentGroup].size()) {
            currentGroup ++;
            currentURLBlock = 0;
            currentCountBlock = 0;
            if (currentGroup >= groups)
                return false;
            readURLs(currentGroup, currentURLBlock);
            readCounts(currentGroup, currentCountBlock);
            return readNext(url, count);
        }
        if (urls.empty()) {
            currentURLBlock ++;
            readURLs(currentGroup, currentURLBlock);
            return readNext(url, count);
        }
        if (counts.empty()) {
            currentCountBlock ++;
            readCounts(currentGroup, currentCountBlock);
            return readNext(url, count);
        }
        url->assign(urls.front());
        *count = counts.front();
        urls.pop_front();
        counts.pop_front();
        return true;
    }
private:
    int groups;
    LRUCache *urlCache;
    LRUCache *countCache;
    std::vector<std::vector<int> > urlOffsets;
    std::vector<long long int> urlNums;

    // for reads
    int currentGroup;
    int currentURLBlock;
    int currentCountBlock;
    std::list<std::string> urls;
    std::list<long long int> counts;

private:
    unsigned int urlHash(const std::string &url) {
        static std::hash<std::string> hash_str;
        // todo
        return hash_str(url);
    }

    void updateCountCache(int group, long long int offset) {
        int blockNum = offset / (BLOCKSIZE / sizeof(long long int));
        int blockOffset = offset % (BLOCKSIZE / sizeof(long long int));
        CountBlock* block = (CountBlock*)countCache->getBlock(group, blockNum);
        block->addOne(blockOffset);
    }

    void appendNewURL(int group, const std::string &url) {
        int lastBlockNum = urlOffsets[group].size() - 1;
        // try using the existed block
        URLBlock* block = (URLBlock*)urlCache->getBlock(group, lastBlockNum);
        if (!block->tryAppendURL(url)) {
            // need new url block
            urlOffsets[group].push_back(urlNums[group]);
            block = (URLBlock*)urlCache->getBlock(group, lastBlockNum + 1);
            assert(block->tryAppendURL(url)); // url is larger than a block
        }
        updateCountCache(group, urlNums[group]++);
    }

    void readURLs(int group, int blockNum) {
        URLBlock* block = (URLBlock*)urlCache->getBlock(group, blockNum);
        urls.splice(urls.end(), block->getURLs());
    }

    void readCounts(int group, int blockNum) {
        CountBlock* block = (CountBlock*)countCache->getBlock(group, blockNum);
        counts.splice(counts.end(), block->getCounts());
    }
};

#endif // DISKURLHASHMAP_H
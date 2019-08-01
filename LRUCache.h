#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <vector>
#include <list>
#include <map>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Block.h"

struct LRUEntry {
    int group;
    int blockNum;
    Block* value;
};

// LRU Cache with groups
class LRUCache {
public:
    LRUCache(int _groups, int _blocks, const std::string _fileName) {
        groups = _groups;
        blocks = _blocks;
        fileNamePrefix = _fileName;
        // initialize blocks
        char *data = new char[BLOCKSIZE * blocks];
        for (int i = 0; i < blocks; i++)
            freeBlocks.push_back(new Block(data + BLOCKSIZE * i));
        cacheIndex.resize(groups);
        // initialize files
        fds.resize(groups);
        for (int i = 0; i < groups; i++) {
            fds[i] = open(getFileName(i).data(), O_CREAT|O_RDWR);
            // write first block
            getBlock(i, 0);
        }
    }

    ~LRUCache() {
        for (auto entry : cacheList)
            entry->value->writeBack(fds[entry->group], entry->blockNum);
        for (auto fd : fds)
            close(fd);
    }

    Block* getBlock(int group, int blockNum) {
        auto cachedBlock = cacheIndex[group].find(blockNum);
        if (cachedBlock != cacheIndex[group].end()) {
            // block is cached 
            auto iter = cachedBlock->second;
            LRUEntry* entry = *iter;
            cacheList.erase(iter);
            cacheList.push_front(entry);
            cacheIndex[group][blockNum] = cacheList.begin();
            return entry->value;
        } else {
            // block is not cached
            auto entry = getNewLRUEntry();
            entry->group = group;
            entry->blockNum = blockNum;
            entry->value->readBlock(fds[group], blockNum);
            cacheList.push_front(entry);
            cacheIndex[group][blockNum] = cacheList.begin();
            return entry->value;
        } 
        return nullptr;
    }

    std::vector<std::pair<Block*, int> > getCachedGroup(int group) {
        std::vector<std::pair<Block*, int> > result;
        for(auto entry : cacheIndex[group])
            result.push_back(std::make_pair((*entry.second)->value, entry.first));
        return std::move(result);
    }
private:
    int groups;
    int blocks;
    std::string fileNamePrefix;
    std::vector<int> fds; // file descriptors
    std::list<Block*> freeBlocks;
    std::list<LRUEntry*> cacheList;
    std::vector<std::map<int, typename std::list<LRUEntry*>::iterator> > cacheIndex; 

private:
    LRUEntry* getNewLRUEntry() {
        if (cacheList.size() != blocks) {
            // cache is not full, allocate new Block 
            auto entry = new LRUEntry();
            entry->value = freeBlocks.front();
            freeBlocks.pop_front();
            return entry;
        } else {
            // erase the last entry
            auto entry = cacheList.back();
            cacheList.pop_back();
            cacheIndex[entry->group].erase(entry->blockNum);
            entry->value->writeBack(fds[entry->group], entry->blockNum);
            return entry;
        }
    }

    std::string getFileName(int group) {
        std::string fileName = fileNamePrefix + std::to_string(group);
        return std::move(fileName);
    }
};

#endif
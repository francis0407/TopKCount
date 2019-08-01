#ifndef BLOCK_H
#define BLOCK_H

#include <string>

#include <string.h>
#include <list>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BLOCKSIZE 4096

class Block {
public:
    Block(char *_data) {
        data = _data;
        needWriteBack = false;
    }
    void reset() {
        for (int i = 0; i < BLOCKSIZE; i++)
            data[i] = 0;
        needWriteBack = false;
    }
    void writeBack(int fd, int blockNum) {
        if (!needWriteBack) return;
        lseek(fd, BLOCKSIZE * blockNum, SEEK_SET);
        write(fd, data, BLOCKSIZE);
        needWriteBack = false;
    }
    void readBlock(int fd, int blockNum) {
        lseek(fd, BLOCKSIZE * blockNum, SEEK_SET);
        int res = read(fd, data, BLOCKSIZE);
        if (res != BLOCKSIZE) {
            // allocate a new block
            reset();
            needWriteBack = true;
            writeBack(fd, blockNum);
        }
        needWriteBack = false;
    }
protected:
    bool needWriteBack;
    char *data;
};

class URLBlock : public Block {
public:
    int findURL(const std::string &url) {
        int len = 0;
        char* urlc = nullptr;
        int offset = 0;
        int result = 0;
        while ((offset = nextURL(offset, &len, &urlc)) != 0) {
            if (len == url.size()) {
                if (strncmp(url.data(), urlc, len) == 0) // match
                    return result;
            }
            result ++;
        }
        return -1; // not found
    }

    std::list<std::string> getURLs() {
        int len = 0;
        char* url = nullptr;
        int offset = 0;
        std::list<std::string> result;
        while ((offset = nextURL(offset, &len, &url)) != 0)
            result.push_back(std::string(url, len));
        return std::move(result);
    }

    bool tryAppendURL(const std::string &url) {
        int len = 0;
        char* urlc = nullptr;
        int offset = 0;
        int lastOffset = 0;
        while((offset = nextURL(offset, &len, &urlc)) != 0)
            lastOffset = offset;
        if (lastOffset + url.size() + sizeof(int) < BLOCKSIZE) {
            int urlLen = (int)url.size();
            *(int*)(data + lastOffset) = urlLen;
            memcpy(data + lastOffset + sizeof(int), url.data(), urlLen);
            needWriteBack = true;
            return true;
        }
        return false;
    }

private:
    int nextURL(int offset, int *len, char **url) {
        if (offset + sizeof(int) >= BLOCKSIZE)
            return 0;
        int _len = *(int*)(data + offset);
        if (_len == 0) return 0;
        *len = _len;
        *url = (data + offset + sizeof(int));
        return offset + _len + sizeof(int);
    }
};

class CountBlock : public Block {
public:
    void addOne(int offset) {
        long long int* ldata = (long long int*) data;
        ldata[offset]++;
        needWriteBack = true;
    }
    std::list<long long int> getCounts() {
        std::list<long long int> result;
        long long int *ldata = (long long int*) data;
        for (int i = 0; i < BLOCKSIZE / sizeof(long long int); i++)
            if (ldata[i] != 0)
                result.push_back(ldata[i]);
        return std::move(result);
    }
};


#endif
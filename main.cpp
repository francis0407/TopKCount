#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <assert.h>

#include "DiskURLHashMap.h"

struct UrlCountCmp {
    template<typename T, typename U>
    bool operator()(T const &left, U const &right) {
        if (left.second > right.second)
            return true;
        return false;
    }
}; 

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: ./TopKURL inputFilePath outputFilePath K slots";
        return 0;
    }
    std::string inputFilePath(argv[1]);
    std::string outputFilePath(argv[2]);
    int k = std::stoi(argv[3]);
    int slots = std::stoi(argv[4]);

    int counttemp = 0;

    // 1. Read the whole file and build the hash map
    std::ifstream inputFile(inputFilePath);
    assert(inputFile.is_open());
    std::ofstream outputFile(outputFilePath);
    assert(outputFile.is_open());

    std::string inputBuf;
    DiskURLHashMap urlCounts(slots);
    while (std::getline(inputFile, inputBuf)) {
        urlCounts.addOne(inputBuf);    
        counttemp ++;
        if (counttemp % 10000 == 0)
            std::cout << "finish " << counttemp << std::endl;
    }
    inputFile.close();

    // 2. Use a heap to build topK
    std::cout << "build topK ..." << std::endl;
    std::string url;
    long long int count = 0;
    std::priority_queue<
        std::pair<std::string, long long int>, 
        std::vector<std::pair<std::string, long long int>>,
        UrlCountCmp> topKUrls;
    urlCounts.prepareForRead();
    while (urlCounts.readNext(&url, &count)) {
        topKUrls.push(std::make_pair(url, count));
        if (topKUrls.size() > k)
            topKUrls.pop();
    }

    // 3. Save the results
    while (!topKUrls.empty()) {
        auto urlCount = topKUrls.top();
        topKUrls.pop();
        outputFile << urlCount.first << " " << urlCount.second << std::endl;
    }
    outputFile.close();
    return 0;
}
#pragma once
#include <queue>
#include <vector>

class ThreadIDManager {
public:
    int allocateID();
    void deallocateID(int id);

private:
    int nextID = 1;
    std::priority_queue<int, std::vector<int>, std::greater<int>> availableIDs;
};
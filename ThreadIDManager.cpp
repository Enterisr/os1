#include "ThreadIDManager.h"

int ThreadIDManager::allocateID() {
    if (availableIDs.empty()) {
        return nextID++;
    }
    int id = availableIDs.top();
    availableIDs.pop();
    return id;
}

void ThreadIDManager::deallocateID(int id) {
    availableIDs.push(id);
}
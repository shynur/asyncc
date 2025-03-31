#include "asyncxx/SyncTask.hpp"
#include <iostream>

SyncTask<int> async_main(const int argc, const char *const argv[]) {
    std::cout << "Arguments: \n";
    for (int i = 0; i < argc; i++)
        std::cout << "\t" << argv[i] << "\n";
    co_return 0;
}

int main(const int argc, const char *const argv[]) { return async_main(argc, argv); }

#include "self.h"
#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace infra_file
{
    bool exeDir(std::filesystem::path& out)
    {
        #if defined(_WIN32)

            char buffer[MAX_PATH];
            DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            if (len == 0 || len == MAX_PATH)
                return false;
            out = fs::path(buffer).parent_path();
            return true;

        #elif defined(__linux__)

            char buffer[1024];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len == -1)
                return false;

            buffer[len] = '\0';
            out = fs::path(buffer).parent_path();
            return true;

        #else

            return false;
            
        #endif
    }
}
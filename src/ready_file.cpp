#include "ready_file.h"

#include <fstream>
#include <iostream>

namespace
{
constexpr char READY_FILE_ARGUMENT[] = "--ready-file=";
} // namespace

ReadyFile &ReadyFile::instance()
{
    static ReadyFile ready_file;
    return ready_file;
}

void ReadyFile::configure_from_arguments(int argc, char *argv[])
{
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument.rfind(READY_FILE_ARGUMENT, 0) == 0)
        {
            path_ = argument.substr(sizeof(READY_FILE_ARGUMENT) - 1);
        }
    }
}

bool ReadyFile::upsert()
{
    if (created_ || path_.empty()) return true;

    std::ofstream ready_file(path_);
    if (!ready_file)
    {
        std::cerr << "Unable to create readiness file: " << path_ << std::endl;
        return false;
    }

    ready_file << "ready\n";
    created_ = true;
    return true;
}

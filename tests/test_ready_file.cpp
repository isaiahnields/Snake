#include "test_helpers.h"

#include "../src/ready_file.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>

int main()
{
    const auto unique_suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string path = "snake-ready-file-test-" + std::to_string(unique_suffix) + ".txt";
    std::string argument = "--ready-file=" + path;
    char test_name[] = "test_ready_file";
    char *test_argv[] = {test_name, &argument[0], nullptr};
    ReadyFile::instance().configure_from_arguments(2, test_argv);

    CHECK(ReadyFile::instance().upsert());

    std::ifstream ready_file(path);
    CHECK(static_cast<bool>(ready_file));
    std::string contents;
    std::getline(ready_file, contents);
    CHECK(contents == "ready");
    ready_file.close();

    std::remove(path.c_str());
    return TEST_RESULT();
}

#pragma once

#include <string>

class ReadyFile
{
  public:
    static ReadyFile &instance();

    void configure_from_arguments(int argc, char *argv[]);
    bool upsert();

  private:
    ReadyFile() = default;

    std::string path_;
    bool created_ = false;
};

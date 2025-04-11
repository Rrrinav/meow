#pragma once

#include <vector>
#include <expected>
#include <string>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>

namespace meow
{
  std::expected<int, std::string> wait_for_process(pid_t pid, std::string name = "");

  pid_t create_process(const std::vector<std::string> &args);

  std::expected<void, std::string> show_file(const std::string &file);
}  // namespace prc

#include <format>
#include <print>
#include <vector>

#include "./procs.hpp"
#include "./utils.hpp"

namespace meow
{
  std::expected<int, std::string> wait_for_process(pid_t pid, std::string name)
  {
    int status = 0;
    pid_t result = waitpid(pid, &status, 0);

    if (result == -1)
      return std::unexpected(std::format("waitpid failed for PID {}", pid));

    if (WIFEXITED(status))
    {
      int exit_code = WEXITSTATUS(status);
      if (exit_code != 0)
      {
        if (name.size() > 0)
          return std::unexpected(std::format("Process {} ( {} ) exited with status {}", name, pid, exit_code) );
        else
          return std::unexpected(std::format("Process {} exited with status {}", pid, exit_code));
      }
      return exit_code;
    }
    else if (WIFSIGNALED(status))
    {
      int sig = WTERMSIG(status);
      return std::unexpected(std::format("Process {} was terminated by signal {}", pid, sig));
    }

    return std::unexpected(std::format("Process {} did not exit normally", pid));
  }

  pid_t create_process(const std::vector<std::string> &args)
  {
    pid_t pid = fork();

    if (pid == -1)
    {
      perror("fork");
      return -1;
    }
    else if (pid == 0)
    {
      std::vector<std::string> expanded_args;

      for (auto &arg : args)
        expanded_args.push_back(meow::expand_paths(arg));

      std::vector<char *> argv;

      for (auto &arg : expanded_args)
        argv.push_back(const_cast<char *>(arg.data()));
      argv.push_back(nullptr);

      execvp(argv[0], argv.data());

      perror("execvp");
      _exit(1);
    }

    return pid;
  }

  std::expected<void, std::string> show_file(const std::string &file, std::string_view backend, std::vector<std::string> options)
  {
    std::vector<std::string> args = {std::string(backend.data()), file};
    for (auto opts: options)
      args.push_back(opts);

    pid_t pid = create_process(args);
    if (pid == -1)
      return std::unexpected("Failed to fork process");

    auto result = wait_for_process(pid, "bat");

    if (!result)
      return std::unexpected(result.error());

    return {};
  }
}

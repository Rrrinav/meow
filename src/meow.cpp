#include "./meow.hpp"

#include <cstddef>
#include <print>

void handle_args(std::vector<std::string> args)
{
  std::size_t NUM_ARGS = args.size() - 1;

  if (NUM_ARGS == 0)
  {
    throw std::runtime_error("TODO: Implement default CLI.");
    return;
  }
  else if (NUM_ARGS == 1)
  {
    if (args[1] == "help" || args[1] == "-h")
    {
      std::println();
      std::println("Usage: {} [options] <args>..", args[0]);
      std::println();
      std::println();
      std::println("  Options without args:");
      std::println("    help                         Show this help message");
      std::println("    version                      Show the version information");
      std::println();
      std::println("  Options with args:");
      std::println("     show <file>/<alias>          cat or bat the file added to meow");
      std::println("     add <path>                   Add a file to meow");
      std::println("     remove <file>                Add a file to meow");
      std::println("     alias <alias> <file>         Alias a file name to be able to call it using alias directly");
      std::println("     remove-alias <alias>         Remove an alias");
      std::println();
      return;
    }
    else if (args[1] == "version" || args[1] == "-v")
    {
      std::println("Version: 0.0.1");
      return;
    }
    else
    {
      std::println(stderr, "Unknown command: ' {} '", args[1]);
    }
  }
  else if (NUM_ARGS == 2)
  {
    meow_core(args);
  }
  else
  {
    throw "Kekw";
  }
}


void meow_core(std::vector<std::string> args)
{
  throw std::runtime_error("TODO: Implement meow core functionality.");
  return;
}

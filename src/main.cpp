#include <vector>
#include <print>

#include "./meow.hpp"

int main(int argc, char *argv[])
{
  try
  {
    handle_args(std::vector<std::string>(argv, argv + argc));
  }
  catch (const std::exception &e)
  {
    std::println(stderr, "{}", e.what());
    return 1;
  }
  catch (...)
  {
    std::println(stderr, "[ERROR]: Some weird throw, idk where from? send patches!");
    return 1;
  }
  return 0;
}

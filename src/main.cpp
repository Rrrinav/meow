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
    std::println(stderr, "[ERROR]: {}", e.what());
    std::println(stderr, "Send patches please 󰇸 ! but maybe it is a genuine issue");
    return 1;
  }
  catch (...)
  {
    std::println(stderr, "[ERROR]: Some weird throw, idk where from? send patches please 󰇸 !");
    return 1;
  }
  return 0;
}

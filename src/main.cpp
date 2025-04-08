#include <vector>
#include <print>

#include "./meow.hpp"
#include "./json.hpp"

int main(int argc, char *argv[])
{
  try
  {
    // People use frameworks for this shit now?
    handle_args(std::vector<std::string>(argv, argv + argc));
  }
  catch (const std::exception &e)
  {
    std::println(stderr, "{}", e.what());
    std::println(stderr, "Send patches please 󰇸 !");
    return 1;
  }
  catch (...)
  {
    std::println(stderr, "[ERROR]: Some weird throw, idk where from? send patches please 󰇸 !");
    return 1;
  }
  return 0;
}

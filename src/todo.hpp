#include <vector>
#include <string>

namespace meow
{
  namespace todo
  {
    void repl();
    void add(std::vector<std::string> args);
    void remove(std::vector<std::string> args);
    void list(std::vector<std::string> args);
  } // namespace todo
}  // namespace meow

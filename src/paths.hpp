#include <string>
#include <vector>

namespace paths 
{
  std::string config_path();

  std::string data_path();

void display_suggestions_horizontal(const std::vector<std::string> &matches, int &lines_used);
void display_suggestions_vertical_limited(const std::vector<std::string> &matches, int &lines_used, int max_lines = 10);
std::string prompt_path(const std::string &prompt = "File path: ", bool use_horizontal = true);
}  // namespace meow

#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

namespace prompt
{
  void enable_raw_mode(termios &original);

  void disable_raw_mode(const termios &original);

  std::vector<std::string> complete_path(const std::string &prefix);

  std::string common_prefix(const std::vector<std::string> &items);

  void move_cursor_up(int n);

  void move_cursor_down(int n);

  void clear_lines_below(int n);

  void redraw_prompt(const std::string &prompt, const std::string &buffer);
  void display_suggestions_horizontal(const std::vector<std::string> &matches, int &lines_used);

  void display_suggestions_vertical_limited(const std::vector<std::string> &matches, int &lines_used, int max_lines = 10);

  std::string prompt_path(const std::string &prompt = "File path: ", bool use_horizontal = true);
}  // namespace prompt

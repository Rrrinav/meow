#include <string>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

namespace meow
{
  void disable_raw_mode();

  void enable_raw_mode();

  void handle_resize(int);

  void setup_resize_handler();

  char read_key();

  enum class Key
  {
    ArrowUp,
    ArrowDown,
    PageUp,
    PageDown,
    Home,
    End,
    Quit,
    Unknown
  };

  Key parse_key();

  std::pair<int, int> terminal_dimensions();

  void clear_screen();

  std::vector<std::string> split_lines(std::string_view str);

  std::vector<std::string> wrap_line(std::string_view line, int width);

  void draw_horizontal_line(int row, int pos = -1, int sym = 0, const std::string &ch = "─", const std::string &color = "");

  void draw_title_bar(int row, std::string_view title, int margin_size);

  std::vector<std::string> rebuild_visible_lines(const std::vector<std::string> &original_lines,
                                                 int term_width,
                                                 bool show_line_numbers,
                                                 int left_padding,
                                                 int &lnw);

  void show_contents(std::string_view content, std::string_view title, int left_padding = 2, bool show_line_numbers = false);
}  // namespace meow

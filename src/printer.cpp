#include <print>
#include <string>

#include <sys/ioctl.h>
#include <unistd.h>

#include "./printer.hpp"
#include "./procs.hpp"

void draw_horizontal_line(std::string ch = "―", const std::string &color = "")
{
  int width = meow::get_terminal_width();
  std::string line;
  for (int i = 0; i < width; ++i) line += ch;
  std::println("{}{}{}", color, line, "\033[0m");
}

void print_file(const std::string & str, const std::string &path)
{
  draw_horizontal_line();
  std::println("File: {}", path);
  draw_horizontal_line();
  std::println("{}", str);
  draw_horizontal_line();
}

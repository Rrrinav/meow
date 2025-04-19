#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <sys/ioctl.h>

#include "./prompter.hpp"

namespace fs = std::filesystem;

int suggestion_lines = 0;

void prompt::enable_raw_mode(termios &original)
{
  termios raw;
  tcgetattr(STDIN_FILENO, &original);
  raw = original;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void prompt::disable_raw_mode(const termios &original) 
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original); 
}

std::vector<std::string> prompt::complete_path(const std::string &prefix)
{
  fs::path p = prefix;
  fs::path dir = p.parent_path().empty() ? "." : p.parent_path();
  std::string base = p.filename().string();
  std::vector<std::string> matches;

  if (!fs::exists(dir))
    return matches;

  for (const auto &entry : fs::directory_iterator(dir))
  {
    std::string name = entry.path().filename().string();
    if (name.starts_with(base))
      matches.emplace_back((dir / name).string());
  }

  return matches;
}

std::string prompt::common_prefix(const std::vector<std::string> &items)
{
  if (items.empty())
    return "";
  return std::ranges::fold_left(items, items.front(),
    [](const std::string &acc, const std::string &s)
    {
      auto mismatch_pair = std::mismatch(acc.begin(), acc.end(), s.begin());
      return std::string(acc.begin(), mismatch_pair.first);
    });
}

void prompt::move_cursor_up(int n)
{
  if (n > 0)
    std::cout << "\033[" << n << "A";
}

void prompt::move_cursor_down(int n)
{
  if (n > 0)
    std::cout << "\033[" << n << "B";
}

void prompt::clear_lines_below(int n)
{
  for (int i = 0; i < n; ++i) std::cout << "\033[E\033[2K";
  move_cursor_up(n);
}

void prompt::redraw_prompt(const std::string &prompt, const std::string &buffer)
{
  std::cout << "\r\033[K" << prompt << buffer << std::flush;
}

void prompt::display_suggestions_horizontal(const std::vector<std::string> &matches, int &lines_used)
{
  if (matches.empty())
    return;

  std::vector<std::string> filtered;
  for (const auto &m : matches)
  {
    std::string fname = fs::path(m).filename().string();
    if (fname.front() != '.')
      filtered.emplace_back(m);
  }
  for (const auto &m : matches)
  {
    std::string fname = fs::path(m).filename().string();
    if (fname.front() == '.')
      filtered.emplace_back(m);
  }

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  int term_width = w.ws_col;

  size_t max_len = std::ranges::max(filtered, {}, [](const std::string &s) { return s.size(); }).size();
  int item_width = static_cast<int>(max_len) + 2;
  int items_per_row = std::max(1, term_width / item_width);

  int items_shown = 0;
  lines_used = 0;

  std::cout << "\n";
  lines_used++;

  for (const auto &entry : filtered)
  {
    std::cout << std::left << std::setw(item_width) << entry;
    if (++items_shown % items_per_row == 0)
    {
      std::cout << "\n";
      if (++lines_used == 10)
        break;
    }
  }

  if (items_shown % items_per_row != 0)
  {
    std::cout << "\n";
    ++lines_used;
  }
}

void prompt::display_suggestions_vertical_limited(const std::vector<std::string> &matches, int &lines_used, int max_lines)
{
  lines_used = 0;
  int to_show = std::min(static_cast<int>(matches.size()), max_lines);

  for (int i = 0; i < to_show; ++i)
  {
    std::cout << " " << matches[i] << "\n";
    lines_used++;
  }

  if (matches.size() > max_lines)
  {
    std::cout << " ... and " << (matches.size() - max_lines) << " more matches\n";
    lines_used++;
  }
}

std::string prompt::prompt_path(const std::string &prompt, bool use_horizontal)
{
  termios original;
  enable_raw_mode(original);
  std::string buffer;
  std::cout << prompt << std::flush;

  while (true)
  {
    char c;
    int bytes_read = read(STDIN_FILENO, &c, 1);

    if (bytes_read == 0 || bytes_read == -1 || c == 4)  // Ctrl+D
    {
      disable_raw_mode(original);
      std::cout << "\nExiting...\n";
      exit(0);
    }

    if (bytes_read != 1)
      break;

    if (c == '\n')
      break;
    else if (c == 127 || c == '\b')
    {
      if (!buffer.empty())
        buffer.pop_back();
    }
    else if (c == '\t')
    {
      auto matches = complete_path(buffer);
      clear_lines_below(suggestion_lines);
      suggestion_lines = 0;

      if (matches.size() == 1)
      {
        buffer = matches[0];
      }
      else if (!matches.empty())
      {
        std::string shared = common_prefix(matches);
        if (shared.size() > buffer.size())
          buffer = shared;

        if (use_horizontal)
          display_suggestions_horizontal(matches, suggestion_lines);
        else
          display_suggestions_vertical_limited(matches, suggestion_lines, 10);

        move_cursor_up(suggestion_lines);
      }
    }
    else
    {
      buffer += c;
    }

    redraw_prompt(prompt, buffer);
  }

  std::cout << "\n";
  disable_raw_mode(original);
  return buffer;
}

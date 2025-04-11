#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <ranges>
#include <print>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>
#include <algorithm>
#include <span>
#include <chrono>
#include <thread>

// Global state
termios original_termios{};
bool resize_flag = false;
bool running = true;

// Terminal handling
void disable_raw_mode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
  std::print("\033[?25h");  // show cursor
}

void enable_raw_mode()
{
  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(disable_raw_mode);
  termios raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  std::print("\033[?25l");  // hide cursor
}

void handle_resize(int) { resize_flag = true; }

void setup_resize_handler()
{
  struct sigaction sa{};
  sa.sa_handler = handle_resize;
  sigaction(SIGWINCH, &sa, nullptr);
}

std::pair<int, int> terminal_dimensions()
{
  winsize w{};
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return {w.ws_col, w.ws_row};
}

void clear_screen() { std::print("\033[2J\033[H"); }

std::vector<std::string> split_lines(std::string_view str)
{
  std::vector<std::string> result;
  result.reserve(std::count(str.begin(), str.end(), '\n') + 1);

  auto lines = str
              | std::views::split('\n')
              | std::views::transform([](auto &&r) { return std::string(std::ranges::begin(r), std::ranges::end(r)); });

  for (const auto &line : lines) result.push_back(line);
  return result;
}

std::vector<std::string> wrap_line(std::string_view line, int width)
{
  if (width <= 0)
    return {std::string(line)};

  std::vector<std::string> result;
  result.reserve((line.length() / width) + 1);

  for (size_t i = 0; i < line.length(); i += width) result.push_back(std::string(line.substr(i, width)));
  return result;
}

char read_key()
{
  char c = 0;
  if (read(STDIN_FILENO, &c, 1) == -1)
    running = false;
  return c;
}

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

Key parse_key()
{
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100))
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    if (ready > 0)
    {
      char c = read_key();
      if (c == 'q' || c == 'Q')
        return Key::Quit;

      if (c == '\033')
      {
        char seq[3] = {0, 0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) == -1)
          return Key::Unknown;

        if (seq[0] == '[')
        {
          if (read(STDIN_FILENO, &seq[1], 1) == -1)
            return Key::Unknown;

          switch (seq[1])
          {
            case 'A':
              return Key::ArrowUp;
            case 'B':
              return Key::ArrowDown;
            case '5':
              read(STDIN_FILENO, &seq[2], 1);
              return Key::PageUp;
            case '6':
              read(STDIN_FILENO, &seq[2], 1);
              return Key::PageDown;
            case 'H':
              return Key::Home;
            case 'F':
              return Key::End;
          }

          // Handle additional sequences
          if (seq[1] == '1' || seq[1] == '7')
          {
            read(STDIN_FILENO, &seq[2], 1);
            if (seq[2] == '~')
              return Key::Home;
          }
          else if (seq[1] == '4' || seq[1] == '8')
          {
            read(STDIN_FILENO, &seq[2], 1);
            if (seq[2] == '~')
              return Key::End;
          }
        }
      }
      return Key::Unknown;
    }

    // No input, check if we need to handle resize
    if (resize_flag)
      return Key::Unknown;  // Force redraw cycle

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return Key::Unknown;
}

// Display functions
void draw_horizontal_line(int row, const std::string &ch = "─", const std::string &color = "")
{
  auto [width, _] = terminal_dimensions();

  // Position cursor and clear line
  std::print("\033[{};1H\033[2K", row);

  // For efficiency, build the line in chunks
  constexpr int CHUNK_SIZE = 64;
  std::string chunk;
  chunk.reserve(CHUNK_SIZE);

  for (int i = 0; i < CHUNK_SIZE; ++i) chunk += ch;

  // Print the color prefix
  std::print("{}", color);

  // Print chunks for efficiency
  for (int i = 0; i < width / CHUNK_SIZE; ++i) std::print("{}", chunk);

  // Print remaining characters
  for (int i = 0; i < width % CHUNK_SIZE; ++i) std::print("{}", ch);

  // Reset color
  std::print("\033[0m");
}

void draw_title_bar(int row, std::string_view title, int margin_size)
{
  std::string margin(margin_size, ' ');
  std::print("\033[{};1H\033[2K", row);
  std::print("{} │ File: {}", margin, title);
}

// Core pager implementation
std::vector<std::string> rebuild_visible_lines(const std::vector<std::string> &original_lines, int term_width, bool show_line_numbers,
                                               int left_padding, int &lnw)
{
  // Early return for empty content
  if (original_lines.empty())
    return {};

  std::vector<std::string> result;
  const int total_lines = original_lines.size();

  // Determine line number width once
  lnw = show_line_numbers ? std::to_string(total_lines).length() : 0;

  // Calculate margin width only once
  const int margin_width = (show_line_numbers ? lnw + 1 : 0) + 1 + left_padding;
  const int content_width = std::max(1, term_width - margin_width);

  // Pre-calculate how many lines we'll need (estimate)
  int estimated_total_lines = 0;
  for (const auto &line : original_lines) estimated_total_lines += (line.length() / content_width) + 1;
  result.reserve(estimated_total_lines);

  // Process each original line
  for (size_t i = 0; i < original_lines.size(); ++i)
  {
    const auto &line = original_lines[i];

    // Only wrap if needed to avoid unnecessary work
    auto wraps = (line.length() <= content_width) ? std::vector<std::string>{std::string(line)} : wrap_line(line, content_width);

    // Cache the line number string to avoid recalculating
    std::string line_number;
    if (show_line_numbers)
    {
      line_number = std::to_string(i + 1);
      line_number = std::string(lnw - line_number.length(), ' ') + line_number;
    }

    // Process each wrapped segment
    for (size_t j = 0; j < wraps.size(); ++j)
    {
      std::string margin;
      if (show_line_numbers)
        if (j == 0)
          margin = line_number + " │ ";
        else
          margin = std::string(lnw, ' ') + " │ ";
      else
        margin = std::string(left_padding, ' ') + "│ ";

      result.push_back(margin + wraps[j]);
    }
  }

  return result;
}

void pager(std::string_view content, std::string_view title)
{
  enable_raw_mode();
  setup_resize_handler();
  running = true;

  // Configuration
  bool show_line_numbers = false;
  int left_padding = 2;

  // Process content
  auto original_lines = split_lines(content);

  // Get initial dimensions
  auto [term_width, term_height] = terminal_dimensions();
  int view_lines = term_height - 5;  // Space for header and footer
  int lnw = 0;

  // Build initial display
  auto visible_lines = rebuild_visible_lines(original_lines, term_width, show_line_numbers, left_padding, lnw);
  int offset = 0;
  int prev_offset = -1;  // Force initial render

  // Track if full redraw is needed
  bool need_full_redraw = true;

  // Main loop
  while (running)
  {
    // Handle terminal resize
    if (resize_flag)
    {
      resize_flag = false;
      std::tie(term_width, term_height) = terminal_dimensions();
      view_lines = term_height - 5;
      visible_lines = rebuild_visible_lines(original_lines, term_width, show_line_numbers, left_padding, lnw);
      need_full_redraw = true;

      // Adjust offset if needed after resize
      if (offset + view_lines > static_cast<int>(visible_lines.size()))
        offset = std::max(0, static_cast<int>(visible_lines.size()) - view_lines);
    }

    // Only redraw if something changed
    if (need_full_redraw || offset != prev_offset)
    {
      // Full screen clear only if needed
      if (need_full_redraw)
        clear_screen();

      // Calculate margin size for consistent display
      int margin_size = show_line_numbers ? lnw : left_padding;

      // Draw header
      if (need_full_redraw)
      {
        draw_horizontal_line(1);
        draw_title_bar(2, title, ( show_line_numbers ?  margin_size : margin_size - 1 ));
        draw_horizontal_line(3);
      }

      // Only update the content area (efficient partial update)
      const int content_start_row = 4;
      const int visible_end = std::min(offset + view_lines, static_cast<int>(visible_lines.size()));

      // Clear content area if offset changed
      if (offset != prev_offset)
        for (int i = 0; i < view_lines; ++i) std::print("\033[{};1H\033[2K", i + content_start_row);

      // Draw content
      for (int i = 0; i < view_lines; ++i)
      {
        int idx = i + offset;
        if (idx < visible_lines.size())
        {
          std::print("\033[{};1H", i + content_start_row);
          std::print("{}", visible_lines[idx]);
        }
      }

      // Draw footer
      if (need_full_redraw)
      {
        draw_horizontal_line(term_height - 1);

        // Draw status bar with position info
        std::print("\033[{};1H\033[2K", term_height);

        // Show scroll percentage
        int percentage = visible_lines.empty() ? 100 : std::min(100, static_cast<int>((offset + view_lines) * 100 / visible_lines.size()));

        std::print("\033[1;38;5;248m ↩ wrapped | ↑↓ PgUp/PgDn | Line: {}/{} ({:3}%) | q:quit\033[0m", offset + 1,
                   visible_lines.empty() ? 1 : visible_lines.size(), percentage);
      }

      need_full_redraw = false;
      prev_offset = offset;
      std::cout.flush();
    }

    // Handle input
    Key key = parse_key();
    switch (key)
    {
      case Key::ArrowUp:
        if (offset > 0)
          offset--;
        break;
      case Key::ArrowDown:
        if (offset + view_lines < static_cast<int>(visible_lines.size()))
          offset++;
        break;
      case Key::PageUp:
        offset = std::max(0, offset - view_lines);
        break;
      case Key::PageDown:
        offset = std::min(static_cast<int>(visible_lines.size()) - view_lines, offset + view_lines);
        break;
      case Key::Home:
        offset = 0;
        break;
      case Key::End:
        offset = std::max(0, static_cast<int>(visible_lines.size()) - view_lines);
        break;
      case Key::Quit:
        running = false;
        break;
      default:
        // Check for resize events during idle
        if (resize_flag)
          continue;

        // Small delay to reduce CPU usage during idle
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break;
    }
  }

  // Cleanup
  clear_screen();
  disable_raw_mode();
}

int main(int argc, char *argv[])
{
  std::string content;
  std::string filename = "meow_pager.txt";

  // If a filename is provided, try to read it
  if (argc > 1)
  {
    filename = argv[1];
    // Here you could add file reading capability
  }
  else
  {
    // Demo content
    std::string line =
        "This is a long line that will be wrapped nicely and prefixed with a left margin. Let's check and do it and make it good and "
        "cool with some longer content to demonstrate wrapping behavior.";

    for (int i = 0; i < 60; ++i) content += "Line " + std::to_string(i + 1) + ": " + line + '\n';
  }

  pager(content, filename);
  return 0;
}

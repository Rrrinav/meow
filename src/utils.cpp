#include "./utils.hpp"


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <print>
#include <iostream>
#include <cstring>

namespace utls
{
  std::optional<std::string> read_file(const std::string &filename)
  {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
      return std::nullopt;

    file.seekg(0, std::ios::end);
    std::size_t size = static_cast<std::size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::string content(size, '\0');
    if (!file.read(content.data(), size))
      return std::nullopt;

    return content;
  }

  void handle_error(const Error &e, bool _exit)
  {
    std::visit(
        [](auto &&err)
        {
          using T = std::decay_t<decltype(err)>;
          if constexpr (std::is_same_v<T, jsn::parse_error>)
          {
            std::println(stderr, "[ERROR]:{}:{}:{}: {}\n        {}", err.location.filename, err.location.line, err.location.column,
                         err.message, err.context);
          }
          else if constexpr (std::is_same_v<T, std::string>)
          {
            std::println(stderr, "[ERROR]: {}", err);
          }
          else
          {
            std::println(stderr, "Unknown error type");
          }
        },
        e);
    if (_exit)
      std::exit(EXIT_FAILURE);
    else
      return;
  }

  std::string expand_paths(std::string_view arg)
  {
    std::string result(arg);

    if (result.starts_with("$HOME"))
    {
      const char *home = std::getenv("HOME");
      if (home)
        result.replace(0, 5, home);
    }
    else if (result.starts_with("~"))
    {
      const char *home = std::getenv("HOME");
      if (home)
        result.replace(0, 1, home);
    }

    return result;
  }

  bool is_path_absolute(std::string_view path)
  {
    if (path.front() == '/' ||
        path.starts_with("~/") ||
        path.starts_with("$HOME/") ||
        path.starts_with("${HOME}/"))
      return true;
    else return false;
  }

  std::expected<void, std::string> write_file(const std::string &filename, const std::string &content)
  {
    std::string temp_filename = filename + ".tmp";

    std::ofstream file(temp_filename, std::ios::out);
    if (!file)
      return std::unexpected("Failed to open temporary file for writing");

    file.write(content.data(), content.size());
    if (file.bad())
      return std::unexpected("Failed to write to temporary file");

    file.close();

    if (std::rename(temp_filename.c_str(), filename.c_str()) != 0)
      return std::unexpected("Failed to rename temporary file to original. New config is in " + temp_filename);

    return {};
  }
}  // namespace utls

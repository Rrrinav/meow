#pragma once

#include <optional>
#include <string>
#include <variant>
#include <expected>

#include "./json.hpp"

namespace meow
{
  std::optional<std::string> read_file(const std::string &filename);

  using Error = std::variant<jsn::parse_error, std::string>;

  void handle_error(const Error &e, bool _exit = true);

  std::string expand_paths(std::string_view arg);

  bool is_path_absolute(std::string_view path);

  std::expected<void, std::string> write_file(const std::string &filename, const std::string &content);

  bool get_json(std::string_view path, jsn::value &config);
}  // namespace utls

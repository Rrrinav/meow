#pragma once

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <fstream>
#include <print>
#include <variant>
#include <iostream>
#include <cstring>

#include "./json.hpp"
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

using Error = std::variant<jsn::parse_error, std::string>;
void handle_error(const Error &e, bool _exit = true)
{
  std::visit(
    [](auto &&err)
    {
    using T = std::decay_t<decltype(err)>;
    if constexpr (std::is_same_v<T, jsn::parse_error>)
    {
      std::println(stderr, "ERROR:{}:{}:{}: {}\n        {}", 
                   err.location.filename,
                   err.location.line,
                   err.location.column,
                   err.message,
                   err.context);
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
      std::println(stderr, "Error: {}", err);
    }
    else
    {
      std::println(stderr, "Unknown error type");
    }
    }, e);
  if(_exit) std::exit(EXIT_FAILURE);
  else return;
}

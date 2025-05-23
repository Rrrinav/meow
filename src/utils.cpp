#include "./utils.hpp"

#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <print>
#include <iostream>
#include <cstring>

namespace meow
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
    {
      std::println("Error reading file: {}", filename);
      return std::nullopt;
    }

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
    if (path.front() == '/' || path.starts_with("~/") || path.starts_with("$HOME/") || path.starts_with("${HOME}/"))
      return true;
    else
      return false;
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

  bool get_json(std::string_view path, jsn::value &config)
  {
    std::filesystem::path _path = std::filesystem::absolute(path);

    if (!std::filesystem::exists(_path))
    {
      std::error_code ec;
      std::filesystem::create_directories(_path.parent_path(), ec);
      if (ec)
      {
        std::println(stderr, "[ERROR]: Failed to create directories for {}: {}", _path.string(), ec.message());
        return false;
      }

      std::ofstream file(_path);
      if (!file)
      {
        std::println(stderr, "[ERROR]: Failed to create file: {}", _path.string());
        return false;
      }

      file << "{\n}";
    }

    std::optional<std::string> json_str = meow::read_file(path.data());
    if (!json_str)
    {
      std::println(stderr, "[ERROR]: Failed to read file: {}", path);
      return false;
    }

    std::expected<jsn::value, jsn::parse_error> config_ex = jsn::try_parse(json_str.value());
    if (!config_ex)
    {
      meow::handle_error(config_ex.error());
      return false;
    }
    config = config_ex.value();
    return true;
  }

  auto ensure_array(jsn::value &data, const std::string &key) -> std::vector<jsn::value> &
  {
    if (!data.exists(key))
      data.add(key, jsn::Value_type::array, jsn::array_type());

    auto &val = data[key];
    if (val.type() == jsn::Value_type::array)
      return val.ref_array();

    if (val.type() != jsn::Value_type::null)
      handle_error(std::format("config file is corrupted: '{}' must be an array", key));

    val = jsn::value::array_type{};
    return val.ref_array();
  }

  void write_data_or_error(const char *path, const jsn::value &data)
  {
    if (auto result = meow::write_file(path, jsn::pretty_print(data, 2)); !result)
      handle_error(std::format("[ERROR]: Failed to write config file: \n     {}", result.error()));
  }
}  // namespace meow

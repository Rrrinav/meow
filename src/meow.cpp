#include <cstddef>
#include <cstdlib>
#include <expected>
#include <optional>
#include <print>
#include <stdexcept>
#include <filesystem>

#include "./meow.hpp"
#include "./utils.hpp"
#include "./procs.hpp"
#include "json.hpp"

void handle_args(std::vector<std::string> args)
{
  std::size_t NUM_ARGS = args.size() - 1;

  if (NUM_ARGS == 0)
  {
    throw std::runtime_error("TODO: Implement default CLI.");
    return void{};
  }
  else if (NUM_ARGS == 1)
  {
    if (args[1] == "help" || args[1] == "-h")
    {
      std::println();
      std::println("Usage: {} [options] <args>..", args[0]);
      std::println();
      std::println();
      std::println("  Options without args:");
      std::println("    help                         Show this help message");
      std::println("    version                      Show the version information");
      std::println();
      std::println("  Options with args:");
      std::println("     show <file>/<alias>          cat or bat the file added to meow");
      std::println("     add <path>                   Add a file to meow");
      std::println("     remove <file>                Add a file to meow");
      std::println("     alias <alias> <file>         Alias a file name to be able to call it using alias directly");
      std::println("     remove-alias <alias>         Remove an alias");
      std::println();
      return;
    }
    else if (args[1] == "version" || args[1] == "-v")
    {
      std::println("Version: 0.0.1");
      return void{};
    }
    else
    {
      std::println(stderr, "Unknown command: ' {} '", args[1]);
      std::println(stderr, "Or maybe you're forgetting args for the command. Try 'help' ");
    }
  }
  else if (NUM_ARGS == 2)
  {
    meow_core(args);
  }
  else
  {
    throw std::runtime_error("TODO: Implement 1 option 2 args functionality");
  }
}

bool get_config(jsn::value &config)
{
  std::optional<std::string> json_config = utls::read_file("./assets/config.json");
  if (!json_config)
  {
    std::println(stderr, "[Error]: Failed to read config file.");
    std::println(stderr, "Please create a config file at ./assets/config.json");
    return false;
  }
  std::expected<jsn::value, jsn::parse_error> config_ex = jsn::try_parse(json_config.value());
  if (!config_ex)
  {
    utls::handle_error(config_ex.error());
    return false;
  }
  config = config_ex.value();
  return true;
}

void meow_core(std::vector<std::string> args)
{
  jsn::value config{};
  if (!get_config(config)) return;

  if (args[1] == "show")
  {
    const std::string FILE = args[2];
    std::vector<jsn::value> files = config["files"].as_array();

    for (jsn::value & f : files)
    {
      if (FILE == f["name"].as_string())
      {
        std::expected<std::string, std::string> path = f["path"].expect_string();
        if (!path)
        {
          utls::handle_error(path.error());
        }
        if (path.value().size() > 0)
        {
          if (auto result = prc::show_file(*path); !result)
          {
            utls::handle_error(result.error());
          }
          return void{};
        }
        return void{};
      }
    }
    utls::handle_error(std::format("File {} not found in config", FILE));
  }
  else if (args[1] == "add")
  {
    const std::string FILE = args[2];
    std::filesystem::path path = std::filesystem::absolute(FILE);

    if (!std::filesystem::exists(path)) utls::handle_error(std::format("File {} does not exist", FILE));

    std::string name = path.filename().string();
    auto files = config["files"].ref_array();

    if (std::find_if(files.begin(), files.end(), [&](const jsn::value &f) { return f["name"].as_string() == name; }) != files.end())
      utls::handle_error(std::format("File name {} already exists", name));

    files.push_back(jsn::value::object_type{
        {"name", name},
        {"path", path.string()},
    });

    if (auto result = utls::write_file("./assets/config.json", jsn::pretty_print(config, 2)); !result)
      utls::handle_error(std::format("[ERROR]: Filed to write config file: \n     {}", result.error()));
  }
  else if (args[1] == "remove")
  {
    std::println("TODO: Implement remove");
  }
  else if (args[1] == "remove-alias")
  {
    std::println("TODO: Implement remove-alias");
  }
}

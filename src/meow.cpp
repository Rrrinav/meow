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
#include "./printer.hpp"
#include "./json.hpp"

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
      std::println("     show <file>/<alias>          cat or bat the file or alias added to meow");
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
      if (args[1] == "--help")
      {
        std::println(stderr, "Unknown command: ' {} '", args[1]);
        std::println(stderr, "Yes I know you want help and yes I won't do it. Use 'help' or '-h' instead");
      }
      else
      {
        std::println(stderr, "Unknown command: ' {} '", args[1]);
        std::println(stderr, "Or maybe you're forgetting args for the command. Try 'help' ");
      }
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


void meow_core(std::vector<std::string> args)
{
  constexpr char CONFIG_PATH[] = "./assets/config.json";
  constexpr char DATA_PATH[] = "./assets/data.json";

  jsn::value config{};
  jsn::value data{};

  if (!meow::get_json(CONFIG_PATH ,config) || !meow::get_json(DATA_PATH ,data))
    return void{};

  if (args[1] == "show")
  {
    const std::string FILE = args[2];
    if (!data.exists("files"))
      data.add("files", jsn::Value_type::array, jsn::array_type({}));

    jsn::Value_type files_type = data["files"].type(); // Idk which way is better
    if (files_type != jsn::Value_type::array)
    {
      if (files_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array"); 
      else
        data["files"] = jsn::value::object_type{};
    }

    std::vector<jsn::value> files = data["files"];

    for (jsn::value & f : files)
    {
      if (FILE == f["name"].as_string())
      {
        std::expected<std::string, std::string> path = f["path"].expect_string();
        if (!path)
        {
          meow::handle_error(path.error());
        }
        if (path.value().size() > 0)
        {
          if (!config.exists("backend"))
          {
            config.add("backend", jsn::Value_type::string, "meow");
          }
          std::string backend = config["backend"].string_opt().value_or("meow");
          if (backend == "cat" || backend == "bat")
          {
            if (auto result = meow::show_file(*path, backend); !result)
            {
              meow::handle_error(result.error());
            }
            return void{};
          }
          else
          {
            meow::show_contents(meow::read_file(meow::expand_paths(path.value())).value_or(""), path.value());
            return void{};
          }
        }
        return void{};
      }
    }
    meow::handle_error(std::format("File {} not found in data\n         Run ' {} help ' to see how to add files", FILE, args[0]));
  }
  else if (args[1] == "add")
  {
    const std::string FILE = args[2];
    std::filesystem::path path = std::filesystem::absolute(FILE);

    if (!std::filesystem::exists(path)) meow::handle_error(std::format("File {} does not exist", FILE));

    std::string name = path.filename().string();
    // If files don't exist, create an empty array
    if (!data.exists("files"))
      data.add("files", jsn::Value_type::array, jsn::array_type({}));

    jsn::Value_type files_type = data["files"].type(); // Idk which way is better
    if (files_type != jsn::Value_type::array)
    {
      if (files_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array"); 
      else
        data["files"] = jsn::value::array_type{};
    }
    std::vector<jsn::value> &files = data["files"].ref_array();

    if (std::find_if(files.begin(), files.end(), [&](const jsn::value &f) { return f["name"].as_string() == name; }) != files.end())
      meow::handle_error(std::format("File name {} already exists", name));

    files.push_back(jsn::value::object_type{
        {"name", name},
        {"path", path.string()},
    });

    if (auto result = meow::write_file(CONFIG_PATH, jsn::pretty_print(config, 2)); !result)
      meow::handle_error(std::format("[ERROR]: Filed to write config file: \n     {}", result.error()));
    else
      std::println("File {} added to meow", name);

    return void{};
  }
  else if (args[1] == "remove")
  {
    const std::string FILE = args[2];
    jsn::Value_type files_type = data["files"].type();
    if (files_type != jsn::Value_type::array)
    {
      if (files_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array");
      else
      {
        std::println("No files found in meow");
        return void{};
      }
    }

    auto &files = data["files"].ref_array();
    std::erase_if(files, [&](const jsn::value &val) { return val["name"].as_string() == FILE; });

    if (auto result = meow::write_file(CONFIG_PATH, jsn::pretty_print(config, 2)); !result)
      meow::handle_error(std::format("[ERROR]: Filed to write config file: \n     {}", result.error()));
    else
      std::println("File {} removed from meow", FILE);

    return void{};
  }
  else if (args[1] == "remove-alias")
  {
    std::println("TODO: Implement remove-alias");
    return void{};
  }
}

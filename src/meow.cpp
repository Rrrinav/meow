#include <algorithm>
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
        std::println(stderr, "Yes I know you want help and yes I won't do it. Use 'help' or '-h' instead.");
      }
      else
      {
        std::println(stderr, "Unknown command: ' {} '", args[1]);
      }
    }
  }
  else if (NUM_ARGS == 2)
  {
    meow_2args(args);
  }
  else if (NUM_ARGS == 3)
  {
    meow_3args(args);
  }
}

// TODO: use std::ranges::find_if or std::find_if wherever possible!
void meow_2args(std::vector<std::string> args)
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

    jsn::Value_type alias_type = data["aliases"].type();
    if (alias_type != jsn::Value_type::array)
    {
      if (alias_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array");
      else
        data["aliases"] = jsn::value::array_type{};
    }

    std::vector<jsn::value> aliases = data["aliases"].ref_array();

    std::vector<jsn::value> files = data["files"];

    auto show = [&](std::string f_name)
    {
      for (const jsn::value &f : files)
      {
        if (f_name == f["name"].as_string())
        {
          std::expected<std::string, std::string> path = f["path"].expect_string();
          if (!path)
            meow::handle_error(path.error());

          if (path.value().size() > 0)
          {
            if (!config.exists("backend"))
              config.add("backend", jsn::Value_type::string, "meow");

            std::string backend = config["backend"].string_opt().value_or("meow");
            if (backend == "cat" || backend == "bat")
            {
              if (auto result = meow::show_file(*path, backend); !result)
                meow::handle_error(result.error());

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
    };
    // If the name doesn't contain a dot '.' i.e an extension , then there's high chance that it's an alias
    if (FILE.find(".") == std::string::npos)
    {
      for (const jsn::value & a : aliases)
      {
        if (a["alias"].as_string() == FILE)
        {
          show(a["file"].as_string());
          return void{};
        }
      }

      show(FILE);
    }
    else
    {
      show(FILE);
      for (const jsn::value & a : aliases)
      {
        if (a["alias"].as_string() == FILE)
        {
          show(a["file"].as_string());
          return void{};
        }
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
      data.add("files", jsn::Value_type::array, jsn::array_type());

    jsn::Value_type files_type = data["files"].type();
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

    if (auto result = meow::write_file(DATA_PATH, jsn::pretty_print(data, 2)); !result)
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

    if (auto result = meow::write_file(DATA_PATH, jsn::pretty_print(data, 2)); !result)
      meow::handle_error(std::format("[ERROR]: Filed to write config file: \n     {}", result.error()));
    else
      std::println("File {} removed from meow", FILE);

    return void{};
  }
  else if (args[1] == "remove-alias")
  {
    if (!data.exists("alias"))
      data.add("alias", jsn::Value_type::array, jsn::array_type());

    jsn::Value_type files_type = data["alias"].type();
    if (files_type != jsn::Value_type::array)
    {
      if (files_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array");
      else
        data["files"] = jsn::value::array_type{};
    }
  }
}

void meow_3args(std::vector<std::string> args)
{
  constexpr char CONFIG_PATH[] = "./assets/config.json";
  constexpr char DATA_PATH[] = "./assets/data.json";

  jsn::value config{};
  jsn::value data{};

  if (!meow::get_json(CONFIG_PATH ,config) || !meow::get_json(DATA_PATH ,data))
    return void{};

  const std::string COMMAND = args[1];

  if (COMMAND == "alias")
  {
    std::string ALIAS = args[2];
    std::string FILE = args[3];

    if (!data.exists("aliases"))
      data.add("aliases", jsn::Value_type::array, jsn::array_type());

    jsn::Value_type alias_type = data["aliases"].type();
    if (alias_type != jsn::Value_type::array)
    {
      if (alias_type != jsn::Value_type::null)
        meow::handle_error("config file is corrupted 'files' is supposed to be an array");
      else
        data["aliases"] = jsn::value::array_type{};
    }

    std::vector<jsn::value> &aliases = data["aliases"].ref_array();

    if (std::find_if(aliases.begin(), aliases.end(), [&](const jsn::value &f) { return f["alias"].as_string() == ALIAS; }) != aliases.end())
      meow::handle_error(std::format("Alias name {} already exists", ALIAS));

    aliases.push_back(jsn::object_type({ {
        { "file", FILE },
        { "alias", ALIAS }
    }}));

    if (auto result = meow::write_file(DATA_PATH, jsn::pretty_print(data, 2)); !result)
      meow::handle_error(std::format("[ERROR]: Filed to write config file: \n     {}", result.error()));
    else
      std::println("Alias {} for {} added to meow", ALIAS, FILE);

    return void{};
  }
  else {
    throw std::runtime_error("Unknown command");
  }

}

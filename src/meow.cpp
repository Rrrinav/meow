#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <optional>
#include <print>
#include <stdexcept>
#include <filesystem>
#include <string>

#include "./meow.hpp"
#include "./utils.hpp"
#include "./procs.hpp"
#include "./printer.hpp"
#include "./json.hpp"
#include "./paths.hpp"

// Lazy-evaluated global config/data paths
const std::string& CONFIG_PATH() {
  static const std::string path = meow::config_path();
  return path;
}

const std::string& DATA_PATH() {
  static const std::string path = meow::data_path();
  return path;
}

void handle_args(std::vector<std::string> args)
{
  std::size_t NUM_ARGS = args.size() - 1;

  if (NUM_ARGS == 0)
    throw std::runtime_error("TODO: Implement default CLI.");

  else if (NUM_ARGS >= 1)
  {
    if (args[1] == "help" || args[1] == "-h")
    {
      std::println();
      std::println("Usage: {} [options] <args>..", args[0]);
      std::println();
      std::println("  Options without args:");
      std::println("     help                         Show this help message");
      std::println("     version                      Show the version information");
      std::println();
      std::println("  Options with args:");
      std::println("     show <file>/<alias>          cat or bat the file or alias added to meow");
      std::println("     add <path>                   Add a file to meow");
      std::println("     remove <file>                Remove a file from meow");
      std::println("     alias <alias> <file>         Alias a file name to call it using alias");
      std::println("     remove-alias <alias>         Remove an alias");
      std::println();
      return;
    }
    else if (args[1] == "version" || args[1] == "-v")
    {
      std::println("Version: 0.0.1");
      return;
    }
    else if (args[1] == "--help")
    {
      std::println(stderr, "Unknown command: ' {} '", args[1]);
      std::println(stderr, "Yes I know you want help and yes I won't do it. Use 'help' or '-h' instead.");
    }
    else if (args[1] == "show")         show_file(args);
    else if (args[1] == "add")          add_file(args);
    else if (args[1] == "remove")       remove_file(args);
    else if (args[1] == "alias")        add_alias(args);
    else if (args[1] == "remove-alias") remove_alias(args);
    else if (args[1] == "open")         open_file(args);
  }
}

// Helper functions
auto ensure_array(jsn::value &data, const std::string &key) -> std::vector<jsn::value> &
{
  if (!data.exists(key))
    data.add(key, jsn::Value_type::array, jsn::array_type());

  auto &val = data[key];
  if (val.type() == jsn::Value_type::array)
    return val.ref_array();

  if (val.type() != jsn::Value_type::null)
    meow::handle_error(std::format("config file is corrupted: '{}' must be an array", key));

  val = jsn::value::array_type{};
  return val.ref_array();
}

void write_data_or_error(const char *path, const jsn::value &data)
{
  if (auto result = meow::write_file(path, jsn::pretty_print(data, 2)); !result)
    meow::handle_error(std::format("[ERROR]: Failed to write config file: \n     {}", result.error()));
}

// show_file
void show_file(std::vector<std::string> args)
{
  if (args.size() != 3)
  {
    std::println(stderr, "Usage: {} show <file>", args[0]);
    return;
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  const std::string FILE = args[2];
  auto &files = ensure_array(data, "files");
  auto &aliases = ensure_array(data, "aliases");

  auto show = [&](const std::string &name)
  {
    auto file = std::ranges::find_if(files, [&](const jsn::value &f) { return f["name"].as_string() == name; });

    if (file == files.end())
      return;

    auto path = (*file)["path"].expect_string();
    if (!path)
      meow::handle_error(path.error());

    std::string backend = config["backend"].string_opt().value_or("meow");
    if (backend == "cat" || backend == "bat")
    {
      if (auto result = meow::show_file(*path, backend); !result)
        meow::handle_error(result.error());
    }
    else
    {
      meow::show_contents(meow::read_file(meow::expand_paths(*path)).value_or(""), *path);
    }
  };

  auto alias_match = std::ranges::find_if(aliases, [&](const jsn::value &a) { return a["alias"].as_string() == FILE; });

  if (alias_match != aliases.end())
    return show((*alias_match)["file"].as_string());
  else
    return show(FILE);

  meow::handle_error(std::format("File {} not found in data\n         Run ' {} help ' to see how to add files", FILE, args[0]));
}

// add_file
void add_file(std::vector<std::string> args)
{
  if (args.size() != 3)
  {
    std::println(stderr, "Usage: {} add <file>", args[0]);
    return;
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  const std::string FILE = args[2];
  std::filesystem::path path = std::filesystem::absolute(FILE);
  if (!std::filesystem::exists(path))
    meow::handle_error(std::format("File {} does not exist", FILE));

  std::string name = path.filename().string();
  auto &files = ensure_array(data, "files");

  if (std::ranges::any_of(files, [&](const jsn::value &f) { return f["name"].as_string() == name; }))
    meow::handle_error(std::format("File name {} already exists", name));

  files.push_back(jsn::object_type{{"name", name}, {"path", path.string()}});
  write_data_or_error(DATA_PATH().c_str(), data);
  std::println("File {} added to meow", name);
}

// remove_file
void remove_file(std::vector<std::string> args)
{
  if (args.size() != 3)
  {
    std::println(stderr, "Usage: {} remove <file>", args[0]);
    return;
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  auto &files = ensure_array(data, "files");
  auto &aliases = ensure_array(data, "aliases");
  const std::string FILE = args[2];
  std::erase_if(files, [&](const jsn::value &val) { return val["name"].as_string() == FILE; });
  std::erase_if(aliases, [&](const jsn::value &val) { return val["alias"].as_string() == FILE; });
  write_data_or_error(DATA_PATH().c_str(), data);
  std::println("File {} removed from meow", FILE);
}

// add_alias
void add_alias(std::vector<std::string> args)
{
  if (args.size() != 4)
  {
    std::println(stderr, "Usage: {} alias <alias> <file>", args[0]);
    return;
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  std::string ALIAS = args[2], FILE = args[3];
  auto &aliases = ensure_array(data, "aliases");

  if (std::ranges::any_of(aliases, [&](const jsn::value &f) { return f["alias"].as_string() == ALIAS; }))
    meow::handle_error(std::format("Alias name {} already exists", ALIAS));

  aliases.push_back(jsn::object_type{{"file", FILE}, {"alias", ALIAS}});
  write_data_or_error(DATA_PATH().c_str(), data);
  std::println("Alias {} for {} added to meow", ALIAS, FILE);
}

// remove_alias
void remove_alias(std::vector<std::string> args)
{
  if (args.size() != 3)
  {
    std::println(stderr, "Usage: {} remove-alias <alias>", args[0]);
    return;
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  auto &aliases = ensure_array(data, "aliases");
  std::string ALIAS = args[2];
  auto original_size = aliases.size();

  std::erase_if(aliases, [&](const jsn::value &entry) { return entry["alias"].as_string() == ALIAS; });

  if (aliases.size() == original_size)
    return std::println(stderr, "[INFO]: Alias '{}' not found.", ALIAS);

  write_data_or_error(DATA_PATH().c_str(), data);
  std::println("Alias '{}' removed from meow", ALIAS);
}

// open_file
void open_file(std::vector<std::string> args)
{
  if (args.size() < 3)
  {
    std::println(stderr, "Usage: {} open <file>", args[0]);
    return;
  }

  std::string FILE = args[2];
  jsn::value config, data;
  if (!meow::get_json(DATA_PATH(), data))
    return;

  ensure_array(data, "files");
  ensure_array(data, "aliases");

  auto &files   = data["files"].ref_array();
  auto &aliases = data["aliases"].ref_array();

  std::optional<std::string> path = std::nullopt;

  auto resolve_file_path = [&](const std::string &name) -> std::optional<std::string>
  {
    auto file = std::ranges::find_if(files, [&](const jsn::value &f) { return f["name"].as_string() == name; });
    if (file != files.end())
      return (*file)["path"].string_opt();
    return std::nullopt;
  };

  if (FILE.find('.') == std::string::npos)
  {
    auto alias = std::ranges::find_if(aliases, [&](const jsn::value &a) { return a["alias"].as_string() == FILE; });

    if (alias != aliases.end())
      path = resolve_file_path((*alias)["file"].string_opt().value_or(""));
    else
      path = resolve_file_path(FILE);
  }
  else
  {
    path = resolve_file_path(FILE);
    if (!path)
    {
      auto alias = std::ranges::find_if(aliases, [&](const jsn::value &a) { return a["alias"].as_string() == FILE; });
      if (alias != aliases.end())
        path = resolve_file_path((*alias)["file"].string_opt().value_or(""));
    }
  }

  if (path)
  {
    const char *editor = std::getenv("EDITOR");
    std::string editor_cmd = editor ? editor : "nano";
    std::string command = std::format("{} '{}'", editor_cmd, *path);
    std::system(command.c_str());
    return;
  }

  std::println(stderr, "File or alias '{}' not found in config", FILE);
}

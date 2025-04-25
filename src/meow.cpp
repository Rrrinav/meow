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
#include "./prompter.hpp"
#include "./todo.hpp"

// Lazy-evaluated global config/data paths
const std::string& CONFIG_PATH() {
  static const std::string path = paths::config_path();
  return path;
}

const std::string& DATA_PATH() {
  static const std::string path = paths::data_path();
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
      std::println();
      std::println("    --------------------File commands--------------------");
      std::println();
      std::println("     open <file>                  Open a file in the default editor");
      std::println("     show <file>/<alias>          cat or bat the file or alias added to meow");
      std::println("     add <path>                   Add a file to meow");
      std::println("     remove <file>                Remove a file from meow");
      std::println("     alias <alias> <file>         Alias a file name to call it using alias");
      std::println("     remove-alias <alias>         Remove an alias");
      std::println();
      std::println("    --------------------TODO commands--------------------");
      std::println();
      std::println("     todo                          Open todo repl");
      std::println("     todo add <todo>               Add a todo");
      std::println("     todo remove <todo no.>        Remove a todo");
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
    else if (args[1] == "show")         return show_file(args);
    else if (args[1] == "add")          return add_file(args);
    else if (args[1] == "remove")       return remove_file(args);
    else if (args[1] == "alias")        return add_alias(args);
    else if (args[1] == "remove-alias") return remove_alias(args);
    else if (args[1] == "open")         return open_file(args);
    else if (args[1] == "todo")         return meow_todo(args);
  }
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
  if (FILE.empty())
    meow::handle_error("File name is empty");

  auto &files = meow::ensure_array(data, "files");
  auto &aliases = meow::ensure_array(data, "aliases");

  auto show = [&](const std::string &name)
  {
    auto file = std::ranges::find_if(files, [&](const jsn::value &f) { return f["name"].as_string() == name; });

    if (file == files.end())
      return;

    auto path = (*file)["path"].expect_string();
    if (!path)
      meow::handle_error(path.error());

    std::string backend = config["backend"].string_opt().value_or("meow");

    if (backend == "bat")
    {
      auto bat_opts = config["bat-options"].array_opt().value_or({});
      std::vector<std::string> options;
      std::ranges::transform(bat_opts, std::back_inserter(options), [](const jsn::value &v) { return v.as_string(); });

      if (auto result = meow::show_file(*path, backend, options); !result)
        meow::handle_error(result.error());
    }
    else if (backend == "cat")
    {
      auto cat_opts = config["cat-options"].array_opt().value_or({});
      std::vector<std::string> options;
      std::ranges::transform(cat_opts, std::back_inserter(options), [](const jsn::value &v) { return v.as_string(); });

      if (auto result = meow::show_file(*path, backend, options); !result)
        meow::handle_error(result.error());
    }
    else
    {
      auto meow_opts = config["meow-options"].array_opt().value_or({});
      bool line_numbers = true;
      int left_pad = 0;
      for (auto meow_opt : meow_opts)
      {
        if (meow_opt.as_object().contains("line-numbers"))
          line_numbers = meow_opt["line-numbers"].as_boolean();
        else if (meow_opt.as_object().contains("left-padding"))
          left_pad = static_cast<int>(meow_opt["left-padding"].as_number());
      }
      meow::show_contents(meow::read_file(meow::expand_paths(*path)).value_or(""), *path, left_pad, line_numbers);
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
  std::optional<std::string> arg2 = std::nullopt;

  if (args.size() < 3)
  {
    arg2 = prompt::prompt_path("Enter file path: ");
    if (!arg2 || arg2.value().empty())
    {
      std::println(stderr, "Error: No value read");
      return;
    }
  }
  if (args.size() > 3)
  {
    meow::handle_error(std::format("Usage: {} add <file>", args[0]));
  }

  jsn::value config, data;
  if (!meow::get_json(CONFIG_PATH(), config) || !meow::get_json(DATA_PATH(), data))
    return;

  std::string _file;
  if (arg2)
   _file = arg2.value();
  else
    _file = args[2];

  std::string FILE = _file;
  if (FILE.empty())
    meow::handle_error("File name is empty");

  std::filesystem::path path = std::filesystem::absolute(FILE);
  if (!std::filesystem::exists(path))
    meow::handle_error(std::format("File {} does not exist", FILE));

  std::string name = path.filename().string();
  auto &files = meow::ensure_array(data, "files");

  if (std::ranges::any_of(files, [&](const jsn::value &f) { return f["name"].as_string() == name; }))
    meow::handle_error(std::format("File name {} already exists", name));

  files.push_back(jsn::object_type{{"name", name}, {"path", path.string()}});
  meow::write_data_or_error(DATA_PATH().c_str(), data);
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

  auto &files = meow::ensure_array(data, "files");
  auto &aliases = meow::ensure_array(data, "aliases");
  const std::string FILE = args[2];
  if (FILE.empty())
    meow::handle_error("File name is empty");

  std::erase_if(files, [&](const jsn::value &val) { return val["name"].as_string() == FILE; });
  std::erase_if(aliases, [&](const jsn::value &val) { return val["alias"].as_string() == FILE; });
  meow::write_data_or_error(DATA_PATH().c_str(), data);
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
  if (ALIAS.empty() || FILE.empty())
    meow::handle_error("Alias or file name is empty");

  auto &aliases = meow::ensure_array(data, "aliases");

  if (std::ranges::any_of(aliases, [&](const jsn::value &f) { return f["alias"].as_string() == ALIAS; }))
    meow::handle_error(std::format("Alias name {} already exists", ALIAS));

  aliases.push_back(jsn::object_type{{"file", FILE}, {"alias", ALIAS}});
  meow::write_data_or_error(DATA_PATH().c_str(), data);
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

  auto &aliases = meow::ensure_array(data, "aliases");
  std::string ALIAS = args[2];
  if (ALIAS.empty())
    meow::handle_error("Alias is empty");

  auto original_size = aliases.size();

  std::erase_if(aliases, [&](const jsn::value &entry) { return entry["alias"].as_string() == ALIAS; });

  if (aliases.size() == original_size)
    return std::println(stderr, "[INFO]: Alias '{}' not found.", ALIAS);

  meow::write_data_or_error(DATA_PATH().c_str(), data);
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
  if (FILE.empty())
    meow::handle_error("File name is empty");

  jsn::value config, data;
  if (!meow::get_json(DATA_PATH(), data))
    return;

  meow::ensure_array(data, "files");
  meow::ensure_array(data, "aliases");

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

void meow_todo(std::vector<std::string> args)
{
  if (args.size() <= 2)
  {
    meow::handle_error(std::format("Usage: {} todo <add|remove|list>", args[0]));
    return;
  }
  else if (args.size() > 2)
  {
    if (args[2] == "add")      return meow::todo::add(args);
    if (args[2] == "remove")   return meow::todo::remove(args);
    if (args[2] == "list")     return meow::todo::list(args);
    if (args[2] == "toggle")   return meow::todo::toggle(args);
    else
    {
      std::println(stderr, "Usage: {} todo <add|remove|list>", args[0]);
      return void{};
    }
  }
  else
  {
    std::println(stderr, "Usage: {} todo <add|remove|list>", args[0]);
    return void{};
  }
}

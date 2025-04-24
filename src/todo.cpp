#include <string>
#include <format>
#include <print>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <optional>
#include <optional>
#include <vector>

#include "./utils.hpp"
#include "./todo.hpp"
#include "./paths.hpp"
#include "./json.hpp"

std::optional<std::chrono::sys_days> parse_date(const std::string &date_str)
{
  std::istringstream ss(date_str);
  std::chrono::year_month_day ymd;
  ss >> std::chrono::parse("%d/%m/%Y", ymd);
  if (ss.fail() || !ymd.ok())
    return std::nullopt;
  return std::chrono::sys_days{ymd};
}

std::string time_left(std::string_view due_date)
{
  std::chrono::sys_days due = parse_date(due_date.data()).value_or(std::chrono::sys_days{});
  auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
  auto diff = due - today;

  if (diff.count() < 0)
    return "past due";

  int days = diff.count();
  return std::format("{} day{}", days, days == 1 ? "" : "s");
}

void meow::todo::add(std::vector<std::string> args)
{
  const int NUM_ARGS = args.size();
  std::string TODO;
  std::string raw_due_date;

  if (NUM_ARGS == 3)
  {
    std::print("Enter todo: ");
    std::getline(std::cin, TODO);
    if (TODO.empty())
      meow::handle_error("Empty todo entered");

    std::print("Enter due-date (dd/mm/yyyy) (optional): ");
    std::getline(std::cin, raw_due_date);
  }
  else if (NUM_ARGS == 4)
  {
    TODO = args[3];
    if (TODO.empty())
      meow::handle_error("Empty todo string provided");

    std::print("Enter due-date (dd/mm/yyyy) (optional): ");
    std::getline(std::cin, raw_due_date);
  }
  else
  {
    meow::handle_error(std::format("Usage: {} todo add <todo string>", args[0]));
    return;
  }

  std::optional<std::chrono::sys_days> parsed_date;
  if (!raw_due_date.empty())
  {
    parsed_date = parse_date(raw_due_date);
    if (!parsed_date)
    {
      meow::handle_error("Invalid date format. Please use dd/mm/yyyy.");
      return;
    }

    auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    if (*parsed_date < today)
    {
      meow::handle_error("Due data has already passed");
      return;
    }
  }

  jsn::value data;
  if (!meow::get_json(paths::data_path(), data)) return;

  meow::ensure_array(data, "todos");

  jsn::value new_todo = jsn::object_type({
    {"todo"    , TODO},
    {"due-date", raw_due_date},
    {"done"    , false}
  });

  auto &todos = data["todos"].ref_array();
  todos.push_back(new_todo);

  meow::write_data_or_error(paths::data_path().c_str(), data);
  std::println("Todo added!");
}

void meow::todo::remove(std::vector<std::string> args)
{
  const int NUM_ARGS = args.size();
  if (NUM_ARGS != 4)
  {
    meow::handle_error(std::format("Usage: {} todo remove <index | todo string>", args[0]));
    return;
  }

  jsn::value data;
  if (!meow::get_json(paths::data_path(), data))
    return;
  meow::ensure_array(data, "todos");
  auto &todos = data["todos"].ref_array();

  std::string token = args[3];
  bool removed = false;
  std::string todo_str = "";
  try
  {
    int index = std::stoi(token);
    if (index < 1 || index > static_cast<int>(todos.size()))
    {
      meow::handle_error("Index out of range");
      return;
    }
    todo_str = todos[index - 1]["todo"].as_string();
    todos.erase(todos.begin() + (index - 1));
    removed = true;
  }
  catch (const std::invalid_argument &)
  {
    // Not a number: treat as todo string
    auto it = std::ranges::find_if(todos, [&](const jsn::value &v) { return v["todo"].as_string() == token; });
    todo_str = token;
    if (it != todos.end())
    {
      todos.erase(it);
      removed = true;
    }
  }

  if (!removed)
  {
    meow::handle_error("No todo found with that index or description.");
    return;
  }

  meow::write_data_or_error(paths::data_path().c_str(), data);
  std::println("Todo {} removed!", todo_str);
}

void meow::todo::list(std::vector<std::string> args)
{
  jsn::value data;
  if (!meow::get_json(paths::data_path(), data))
    return;

  meow::ensure_array(data, "todos");
  const auto &todos = data["todos"].as_array();

  if (todos.empty())
  {
    std::cout << "\033[1;34mYour todo list is empty. Time to relax!\033[0m\n";
    return;
  }

  std::cout << "\n\033[1;33m───────────────────────────── 󱙵  Your todos 󱙵  ─────────────────────────────\033[0m\n\n";

  int index = 1;
  for (int i = 0; i < todos.size(); ++i)
  {
    auto item = todos[i];
    const auto &obj = item.as_object();

    std::string text = obj.at("todo").as_string();
    std::string due_date = obj.at("due-date").as_string();
    bool done = obj.at("done").as_boolean();
    std::string checkbox = done ? "\033[1;32m[✓]\033[0m" : "\033[1;31m[ ]\033[0m";

    // Add color and emphasis to the todo text
    std::string todo_style = done ? "\033[1;32m" : "\033[1;37m";  // Green if done, white if not
    std::string styled_text = todo_style + text + "\033[0m";

    std::string timeleft = time_left(due_date);
    bool invalid_time = (timeleft == "invalid date");

    if (i == 0)
      std::cout << " \033[2m  ┌─────────────────────────────────────────────────────────────\033[0m\n";

    std::cout << " \033[2m\033[0m " << std::setw(2) << index++ << ". " << checkbox << " " << styled_text << "\n";

    std::cout << " \033[2m\033[0m           \033[34mdue-date :\033[0m " << due_date << "\n";

    std::cout << " \033[2m\033[0m           \033[34mtime-left:\033[0m " << (invalid_time ? "\033[2;31minvalid date\033[0m" : timeleft) << "\n";
    if (todos.size() - 1 == i) continue;
    std::cout << " \033[2m    ────────────────────────────────────────────────────────────\033[0m\n";
  }
  std::cout << " \033[2m  └─────────────────────────────────────────────────────────────\033[0m\n";
}

void meow::todo::toggle(std::vector<std::string> args)
{
  const int NUM_ARGS = args.size();
  if (NUM_ARGS != 4)
  {
    meow::handle_error(std::format("Usage: {} todo toggle <index | todo string>", args[0]));
    return;
  }

  jsn::value data;
  if (!meow::get_json(paths::data_path(), data))
    return;
  meow::ensure_array(data, "todos");
  auto &todos = data["todos"].ref_array();

  std::string token = args[3];
  bool changed = false;
  std::string todo_str = "";
  std::string status = "done";
  try
  {
    int index = std::stoi(token);
    if (index < 1 || index > static_cast<int>(todos.size()))
    {
      meow::handle_error("Index out of range");
      return;
    }
    todo_str = todos[index - 1]["todo"].as_string();
    auto& todo = todos[index - 1].ref_object();
    todo["done"] = !todo["done"].as_boolean();
    changed = true;
    status = todo["done"].as_boolean() ? "done" : "not done";
  }
  catch (const std::invalid_argument &)
  {
    // Not a number: treat as todo string
    auto it = std::ranges::find_if(todos, [&](const jsn::value &v) { return v["todo"].as_string() == token; });
    todo_str = token;
    if (it != todos.end())
    {
      auto& todo = it->ref_object();
      todo["done"] = !todo["done"].as_boolean();
      changed = true;
      status = todo["done"].as_boolean() ? "done" : "not done";
    }
  }
  if (!changed)
  {
    meow::handle_error("No todo found with that index or description.");
    return;
  }
  meow::write_data_or_error(paths::data_path().c_str(), data);
  std::println("Todo {} marked as {}!", todo_str, status);
  return void{};
}

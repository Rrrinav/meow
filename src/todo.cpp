#include <string>
#include <format>
#include <print>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <optional>

#include "./utils.hpp"
#include "./todo.hpp"
#include "./paths.hpp"
#include "json.hpp"

void meow::todo::repl()
{
  throw std::runtime_error("TODO: Implement todo repl");
}

std::optional<std::chrono::sys_days> parse_date(const std::string &date_str)
{
  std::istringstream ss(date_str);
  std::chrono::year_month_day ymd;
  ss >> std::chrono::parse("%d/%m/%Y", ymd);
  if (ss.fail() || !ymd.ok())
    return std::nullopt;
  return std::chrono::sys_days{ymd};
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
  if (!meow::get_json(paths::data_path(), data))
    return;

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
  throw std::runtime_error("TODO: Implement todo repl");
}

void meow::todo::list(std::vector<std::string> args)
{
  throw std::runtime_error("TODO: Implement todo repl");
}

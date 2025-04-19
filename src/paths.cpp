#include "./paths.hpp"

std::string paths::config_path()
{
  const char *xdg = std::getenv("XDG_CONFIG_HOME");
  std::string base = xdg ? xdg : std::string(std::getenv("HOME")) + "/.config";
  return base + "/meow/config.json";
}

std::string paths::data_path()
{
  const char *xdg = std::getenv("XDG_DATA_HOME");
  std::string base = xdg ? xdg : std::string(std::getenv("HOME")) + "/.local/share";
  return base + "/meow/data.json";
}

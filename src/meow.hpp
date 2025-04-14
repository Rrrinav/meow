#pragma once

#include <vector>
#include <string>

#include "./json.hpp"

void handle_args(std::vector<std::string> args);
bool get_config(jsn::value &config);
void meow_2args(std::vector<std::string> args);
void meow_3args(std::vector<std::string> args);

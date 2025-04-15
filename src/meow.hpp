#pragma once

#include <vector>
#include <string>

#include "./json.hpp"

void handle_args(std::vector<std::string> args);
bool get_config(jsn::value &config);
void show_file(std::vector<std::string> args);
void add_file(std::vector<std::string> args);
void remove_file(std::vector<std::string> args);
void add_alias(std::vector<std::string> args);
void remove_alias(std::vector<std::string> args);
void meow_2args(std::vector<std::string> args);
void meow_3args(std::vector<std::string> args);

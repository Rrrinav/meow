#include <cassert>
#include <cstdlib>
#include <string>

#define B_LDR_IMPLEMENTATION
#define BLD_USE_CONFIG
#include "./b_ldr.hpp"

std::string get_compiler_name()
{
#if defined(__clang__)
  return "clang++";
#elif defined(__GNUC__)
  return "g++";
#elif defined(_MSC_VER)
  return "msvc";
#else
  return "unknown";
#endif
}

std::string get_compiler_version()
{
#if defined(__clang__)
  return std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
  return std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
  return std::to_string(_MSC_VER);
#else
  return "unknown";
#endif
}

const std::string COMPILER_NAME = get_compiler_name();
const std::string COMPILER_VERSION = get_compiler_version();
const std::string BUILD_FOLDER = "./build/";
const std::string SRC_FOLDER = "./src/";
const std::string OBJ_FOLDER = BUILD_FOLDER + "obj/";
const std::string SRC_FILE = "main.cpp";
const std::string CPP_STD = "--std=c++23";
const std::string EXECUTABLE = "main";

void handle_run()
{
  if (bld::execute_shell(BUILD_FOLDER + EXECUTABLE) < 0)
  {
    bld::log(bld::Log_type::ERR, "Failed to run the executable.");
    exit(1);
  }
  exit(0);
}

void handle_clean()
{
  if (!bld::fs::remove_dir(BUILD_FOLDER))
    exit(1);
  exit(0);
}

void handle_objs()
{
  bld::fs::create_dir_if_not_exists(OBJ_FOLDER);
  std::vector<std::string> files = bld::fs::list_files_in_dir(SRC_FOLDER);
  std::vector<std::string> cpp_files;

  std::copy_if(files.begin(), files.end(), std::back_inserter(cpp_files),
               [](const std::string &f) { return bld::fs::get_extension(f) == ".cpp"; });

  for (const auto &f : cpp_files)
  {
    std::string stem = bld::fs::get_stem(f);
    std::string object_path = OBJ_FOLDER + stem + ".o";

    if (bld::is_executable_outdated(f, object_path))
    {
      bld::log(bld::Log_type::INFO, "Compiling " + f + " to " + object_path);
      if (bld::execute(bld::Command{COMPILER_NAME, "-c", f, "-o", object_path, CPP_STD}) < 0)
        exit(EXIT_FAILURE);
    }
    else
    {
      bld::log(bld::Log_type::INFO, "Object file " + object_path + " is up to date.");
    }
  }
}

int main(int argc, char **argv)
{
  BLD_REBUILD_YOURSELF_ONCHANGE();

  // Optional compiler warnings
#if defined(__GNUC__) && !defined(__clang__)
  if (__GNUC__ < 13)
    bld::log(bld::Log_type::WARNING, "Your g++ version may not support full C++23.");
#elif defined(__clang__)
  if (__clang_major__ < 17)
    bld::log(bld::Log_type::WARNING, "Your clang++ version may not support full C++23.");
#endif

  std::vector<std::string> args;
  if (bld::args_to_vec(argc, argv, args))
  {
    if (args.size() == 1)
    {
      if (args[0] == "run")
      {
        handle_run();
        return 0;
      }
      else if (args[0] == "clean")
      {
        handle_clean();
        return 0;
      }

      bld::log(bld::Log_type::ERR, "Only 'run' and 'clean' commands are supported.");
      return 1;
    }
    else if (args.size() > 1)
    {
      bld::log(bld::Log_type::ERR, "Invalid argument count.\n    Only 'run' and 'clean' commands are supported.");
      return 1;
    }
  }

  bld::fs::create_dir_if_not_exists(BUILD_FOLDER);
  handle_objs();

  auto objs = bld::fs::list_files_in_dir(OBJ_FOLDER);

  bld::Command cmd = {COMPILER_NAME, "-o", BUILD_FOLDER + EXECUTABLE};
  for (const auto &obj : objs) cmd.add_parts(obj);
  cmd.add_parts(CPP_STD);

  if (bld::execute(cmd) < 0)
    return 1;

  return 0;
}

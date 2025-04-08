#include "./json.hpp"

#include <cstdio>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <cctype>
#include <expected>
#include <format>
#include <print>

namespace jsn
{
  Value_type value::get_type_from_index() const
  {
    switch (data.index())
    {
      case 0:
        return Value_type::null;
      case 1:
        return Value_type::boolean;
      case 2:
        return Value_type::number;
      case 3:
        return Value_type::string;
      case 4:
        return Value_type::array;
      case 5:
        return Value_type::object;
      default:
        throw std::runtime_error("Invalid variant index");
    }
  }

  std::string_view value::type_to_string(Value_type type) const
  {
    switch (type)
    {
      case Value_type::null:
        return "null";
      case Value_type::boolean:
        return "boolean";
      case Value_type::number:
        return "number";
      case Value_type::string:
        return "string";
      case Value_type::array:
        return "array";
      case Value_type::object:
        return "object";
      default:
        return "unknown";
    }
  }

  // Constructors
  value::value() noexcept : data(std::monostate{}) {}
  value::value(std::nullptr_t) noexcept : data(std::monostate{}) {}
  value::value(bool val) noexcept : data(val) {}

  // String constructors
  value::value(const char *val) : data(std::string(val)) {}
  value::value(std::string_view val) : data(std::string(val)) {}
  value::value(const std::string &val) : data(val) {}
  value::value(std::string &&val) noexcept : data(std::move(val)) {}

  // Container constructors
  value::value(const array_type &val) : data(val) {}
  value::value(array_type &&val) noexcept : data(std::move(val)) {}
  value::value(const object_type &val) : data(val) {}
  value::value(object_type &&val) noexcept : data(std::move(val)) {}

  // Type checking methods
  Value_type value::type() const noexcept { return get_type_from_index(); }
  bool value::is_null() const noexcept { return std::holds_alternative<std::monostate>(data); }
  bool value::is_boolean() const noexcept { return std::holds_alternative<bool>(data); }
  bool value::is_number() const noexcept { return std::holds_alternative<double>(data); }
  bool value::is_string() const noexcept { return std::holds_alternative<std::string>(data); }
  bool value::is_array() const noexcept { return std::holds_alternative<array_type>(data); }
  bool value::is_object() const noexcept { return std::holds_alternative<object_type>(data); }

  // Conversion operators
  value::operator bool() const { return as_boolean(); }
  value::operator double() const { return as_number(); }
  value::operator std::string() const { return as_string(); }
  value::operator int() const { return static_cast<int>(as_number()); }
  value::operator array_type() const { return as_array(); }
  value::operator object_type() const { return as_object(); }

  bool value::as_boolean() const
  {
    if (!is_boolean())
      throw std::runtime_error(std::format("Type error: expected boolean, got {}", type_to_string(type())));

    return std::get<bool>(data);
  }

  double value::as_number() const
  {
    if (!is_number())
      throw std::runtime_error(std::format("Type error: expected number, got {}", type_to_string(type())));

    return std::get<double>(data);
  }

  const std::string &value::as_string() const
  {
    if (!is_string())
      throw std::runtime_error(std::format("Type error: expected string, got {}", type_to_string(type())));

    return std::get<std::string>(data);
  }

  const value::array_type &value::as_array() const
  {
    if (!is_array())
      throw std::runtime_error(std::format("Type error: expected array, got {}", type_to_string(type())));

    return std::get<array_type>(data);
  }

  const value::object_type &value::as_object() const
  {
    if (!is_object())
      throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

    return std::get<object_type>(data);
  }

  // Array element access
  const value value::operator[](const std::size_t index) const
  {
    if (!is_array())
      throw std::runtime_error(std::format("Type error: expected array, got {}", type_to_string(type())));
    const auto &arr = std::get<array_type>(data);
    if (index >= arr.size())
      return value();
    return arr[index];
  }

  const value value::operator[](int index) const { return (*this)[static_cast<std::size_t>(index)]; }

  // For string literals and const char*
  const value value::operator[](const char *key) const
  {
    if (!is_object())
      throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

    const auto &obj = std::get<object_type>(data);
    auto it = obj.find(key);
    if (it == obj.end())
      return value();

    return it->second;
  }

  // For std::string
  const value value::operator[](const std::string &key) const
  {
    if (!is_object())
      throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

    const auto &obj = std::get<object_type>(data);
    auto it = obj.find(key);
    if (it == obj.end())
      return value();

    return it->second;
  }

  // Safe access with std::expected (C++23)
  std::expected<bool, std::string> value::expect_boolean() const noexcept
  {
    if (!is_boolean())
      return std::unexpected(std::format("Type error: expected boolean, got {}", type_to_string(type())));

    return std::get<bool>(data);
  }

  std::expected<double, std::string> value::expect_number() const noexcept
  {
    if (!is_number())
      return std::unexpected(std::format("Type error: expected number, got {}", type_to_string(type())));

    return std::get<double>(data);
  }

  std::expected<std::string, std::string> value::expect_string() const noexcept
  {
    if (!is_string())
      return std::unexpected(std::format("Type error: expected string, got {}", type_to_string(type())));

    return std::string(std::get<std::string>(data));
  }

  std::expected<value::array_type, std::string> value::expect_array() const noexcept
  {
    if (!is_array())
      return std::unexpected(std::format("Type error: expected array, got {}", type_to_string(type())));

    return std::get<array_type>(data);
  }

  std::expected<value::object_type, std::string> value::expect_object() const noexcept
  {
    if (!is_object())
      return std::unexpected(std::format("Type error: expected object, got {}", type_to_string(type())));

    return std::get<object_type>(data);
  }

  // Optional access versions
  std::optional<bool> value::boolean_opt() const noexcept
  {
    if (!is_boolean())
      return std::nullopt;

    return std::get<bool>(data);
  }

  std::optional<double> value::number_opt() const noexcept
  {
    if (!is_number())
      return std::nullopt;

    return std::get<double>(data);
  }

  std::optional<std::string> value::string_opt() const noexcept
  {
    if (!is_string())
      return std::nullopt;

    return std::string(std::get<std::string>(data));
  }

  std::optional<value::array_type> value::array_opt() const noexcept
  {
    if (!is_array())
      return std::nullopt;

    return std::get<array_type>(data);
  }

  std::optional<value::object_type> value::object_opt() const noexcept
  {
    if (!is_object())
      return std::nullopt;

    return std::get<object_type>(data);
  }

  value::object_type &value::as_object()
  {
    if (!is_object())
      throw std::runtime_error("Value is not an object");
    return std::get<object_type>(data);
  }

  void value::set(const std::string &key, const value &val)
  {
    try
    {
      auto &obj = as_object();
      auto it = obj.find(key);
      if (it != obj.end())
        it->second = val;
      else
        obj[key] = val;
    }
    catch (const std::exception &e)
    {
      throw std::runtime_error(std::format("Failed to set value: {}", e.what()));
    }
  }

  void value::set_nested(const std::string &path, const value &val)
  {
    if (path.empty())
      throw std::invalid_argument("Path cannot be empty");

    // Split the path by dots to get individual keys
    std::vector<std::string> keys;
    std::string current_key;
    std::istringstream path_stream(path);

    while (std::getline(path_stream, current_key, '.')) keys.push_back(current_key);

    if (keys.empty())
      throw std::invalid_argument("Invalid path format");

    // Navigate through the nested structure
    if (!is_object())
    {
      // Convert to an empty object if this isn't already an object
      data = object_type{};
    }

    object_type *current_obj = &std::get<object_type>(data);

    // Navigate to the second-to-last key
    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
      const auto &key = keys[i];

      // If the key doesn't exist, create a new object
      if (current_obj->find(key) == current_obj->end() || !(*current_obj)[key].is_object())
        (*current_obj)[key] = object_type{};

      // Move to the next level
      current_obj = &std::get<object_type>((*current_obj)[key].data);
    }

    // Set the value at the final key
    (*current_obj)[keys.back()] = val;
  }

  std::expected<size_t, std::string> value::push(std::string path, const value &val)
  {
    if (path.empty())
      return std::unexpected("Path cannot be empty");

    // Parse the path into tokens
    std::vector<std::pair<bool, std::string>> tokens;  // pair of (is_array_access, key/index)

    size_t pos = 0;
    std::string current_token;

    while (pos < path.size())
    {
      if (path[pos] == '.')
      {
        // End of an object property
        if (!current_token.empty())
        {
          tokens.push_back({false, current_token});
          current_token.clear();
        }
        pos++;
      }
      else if (path[pos] == '[')
      {
        // Start of an array index
        if (!current_token.empty())
        {
          tokens.push_back({false, current_token});
          current_token.clear();
        }

        // Extract the index
        size_t close_bracket = path.find(']', pos);
        if (close_bracket == std::string::npos)
          return std::unexpected("Unclosed bracket in path: " + path);

        std::string index_str = path.substr(pos + 1, close_bracket - pos - 1);

        // Validate that index is numeric
        if (index_str.empty() || !std::all_of(index_str.begin(), index_str.end(), [](char c) { return std::isdigit(c); }))
          return std::unexpected("Invalid array index '" + index_str + "' in path: " + path);

        tokens.push_back({true, index_str});
        pos = close_bracket + 1;

        // Skip the dot after the bracket if present
        if (pos < path.size() && path[pos] == '.')
          pos++;
      }
      else
      {
        // Part of a property name
        current_token += path[pos++];
      }
    }

    // Add the last token if there is one
    if (!current_token.empty())
      tokens.push_back({false, current_token});

    if (tokens.empty())
      return std::unexpected("Invalid path format");

    // Navigate through the nested structure
    value *current = this;

    // Traverse the path to find the target array
    for (const auto &[is_array_access, token] : tokens)
    {
      if (is_array_access)
      {
        // Handle array access
        if (!current->is_array())
        {
          // Auto-convert to array if needed
          current->data = array_type{};
        }

        auto &arr = std::get<array_type>(current->data);
        size_t index = std::stoull(token);

        // Resize array if needed
        if (index >= arr.size())
          arr.resize(index + 1);

        current = &arr[index];
      }
      else
      {
        // Handle object access
        if (!current->is_object())
        {
          // Auto-convert to object if needed
          current->data = object_type{};
        }

        auto &obj = std::get<object_type>(current->data);

        // Create key if it doesn't exist
        if (obj.find(token) == obj.end())
          obj[token] = value{};

        current = &obj[token];
      }
    }

    // Once we've reached the target, ensure it's an array and push the value
    if (!current->is_array())
    {
      // Convert to array if needed
      current->data = array_type{};
    }

    auto &arr = std::get<array_type>(current->data);
    arr.push_back(val);

    // Return the index where the value was inserted
    return arr.size() - 1;
  }

  std::expected<size_t, std::string> value::put_at(std::string path, const value &val)
  {
    if (path.empty())
      return std::unexpected(std::format("Path cannot be empty"));
    if (path.back() != ']')
      return std::unexpected(std::format("Path must end with an array index"));

    std::vector<std::pair<bool, std::string>> tokens;
    size_t pos = 0;
    std::string current_token;

    while (pos < path.size())
    {
      if (path[pos] == '.')
      {
        if (!current_token.empty())
        {
          tokens.emplace_back(false, current_token);
          current_token.clear();
        }
        pos++;
      }
      else if (path[pos] == '[')
      {
        if (!current_token.empty())
        {
          tokens.emplace_back(false, current_token);
          current_token.clear();
        }

        size_t close_bracket = path.find(']', pos);
        if (close_bracket == std::string::npos)
          return std::unexpected(std::format("Unclosed '[' in path: {}", path));

        std::string index_str = path.substr(pos + 1, close_bracket - pos - 1);
        if (index_str.empty() || !std::all_of(index_str.begin(), index_str.end(), ::isdigit))
          return std::unexpected(std::format("Invalid array index '{}' in path: {}", index_str, path));

        tokens.emplace_back(true, index_str);
        pos = close_bracket + 1;

        if (pos < path.size() && path[pos] == '.')
          pos++;
      }
      else
      {
        current_token += path[pos++];
      }
    }

    if (!current_token.empty())
      return std::unexpected(std::format("Invalid trailing property in path: {}", current_token));

    value *current = this;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
      const auto &[is_array, key] = tokens[i];
      bool is_last = (i == tokens.size() - 1);

      if (is_array)
      {
        size_t index = std::stoull(key);

        if (!current->is_array())
        {
          if (!current->is_null())
            return std::unexpected(std::format("Expected array at index [{}] but found {}", key, type_to_string(current->type())));
          current->data = array_type{};
        }

        auto &arr = std::get<array_type>(current->data);
        if (index >= arr.size())
          arr.resize(index + 1);

        if (is_last)
        {
          arr[index] = val;
          return index;
        }

        current = &arr[index];
      }
      else
      {
        if (!current->is_object())
        {
          if (!current->is_null())
            return std::unexpected(std::format("Expected object at '{}' but found {}", key, type_to_string(current->type())));
          current->data = object_type{};
        }

        auto &obj = std::get<object_type>(current->data);
        current = &obj[key];
      }
    }

    return std::unexpected("Invalid path or unexpected failure.");
  }

  inline json_location compute_location(std::string_view input, size_t pos, const std::string &filename)
  {
    json_location loc;
    loc.position = pos;
    loc.filename = filename;

    // Count newlines to determine line number and column
    size_t line_start = 0;
    for (size_t i = 0; i < pos && i < input.size(); ++i)
    {
      if (input[i] == '\n')
      {
        loc.line++;
        line_start = i + 1;
      }
    }

    loc.column = pos - line_start + 1;
    return loc;
  }

  parse_error::parse_error(const std::string &message, const json_location &loc, std::string_view ctx)
      : std::runtime_error(std::format("{}:{}:{}: {} (context: '{}')", loc.filename, loc.line, loc.column, message, ctx)),
        message(message),
        location(loc),
        context(ctx)
  {
  }

  void parser::skip_whitespace() noexcept
  {
    while (pos < input.size() && std::isspace(input[pos])) pos++;
  }

  bool parser::is_end() const noexcept { return pos >= input.size(); }

  std::string parser::get_context(size_t length) const noexcept
  {
    int begin_pos = pos, end_pos = pos;
    for (size_t i = begin_pos; i > 0; --i)
    {
      if (input[i] == '\n')
      {
        begin_pos = i + 1;
        break;
      }
    }
    for (size_t i = end_pos; i < input.size(); ++i)
    {
      if (input[i] == '\n')
      {
        end_pos = i;
        break;
      }
    }
    // trim whitespace from start and end
    for (size_t i = begin_pos; i < end_pos; ++i)
    {
      if (std::isalnum(input[i]))
        break;
      if (input[i] == '\r' || input[i] == '\t' || std::isspace(input[i]))
        continue;
      begin_pos = i;
      break;
    }
    return std::string(input.substr(begin_pos, end_pos - begin_pos));
  }

  parse_error parser::make_error(const std::string &message) const
  {
    json_location loc = compute_location(input, pos, filename);
    return parse_error(message, loc, get_context());
  }

  std::string parser::parse_string()
  {
    if (input[pos] != '"')
      throw make_error("Expected string");
    pos++;

    std::string result;
    while (pos < input.size() && input[pos] != '"')
    {
      if (input[pos] == '\\' && pos + 1 < input.size())
      {
        pos++;
        switch (input[pos])
        {
          case '"':
            result.push_back('"');
            break;
          case '\\':
            result.push_back('\\');
            break;
          case '/':
            result.push_back('/');
            break;
          case 'b':
            result.push_back('\b');
            break;
          case 'f':
            result.push_back('\f');
            break;
          case 'n':
            result.push_back('\n');
            break;
          case 'r':
            result.push_back('\r');
            break;
          case 't':
            result.push_back('\t');
            break;
          case 'u':
          {
            std::println(stderr, "Unicode not supported");
            exit(EXIT_FAILURE);
            // Unicode handling (simplified)
            if (pos + 4 >= input.size())
              throw make_error("Incomplete Unicode escape sequence");
            // Convert hex digits to code point
            std::string hex = std::string(input.substr(pos + 1, 4));
            pos += 4;  // Skip the 4 hex digits
            // Basic implementation - would need more for full Unicode support
            char16_t code_point = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
            // This is simplified UTF-16 to UTF-8 conversion
            if (code_point < 0x80)
            {
              result.push_back(static_cast<char>(code_point));
            }
            else if (code_point < 0x800)
            {
              result.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
              result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            }
            else
            {
              result.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
              result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
              result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            }
            break;
          }
          default:
            throw make_error(std::format("Invalid escape sequence '\\{}'", input[pos]));
        }
      }
      else
      {
        result.push_back(input[pos]);
      }
      pos++;
    }

    if (pos >= input.size() || input[pos] != '"')
      throw make_error("Unterminated string");
    pos++;  // Skip closing quote

    return result;
  }

  double parser::parse_number()
  {
    size_t start = pos;

    // Handle negative numbers
    if (input[pos] == '-')
      pos++;

    // Integer part
    if (input[pos] == '0')
      pos++;
    else if (std::isdigit(input[pos]))
      while (pos < input.size() && std::isdigit(input[pos])) pos++;
    else
      throw make_error("Invalid number");

    // Fractional part
    if (pos < input.size() && input[pos] == '.')
    {
      pos++;
      if (pos >= input.size() || !std::isdigit(input[pos]))
        throw make_error("Expected digit after decimal point");
      while (pos < input.size() && std::isdigit(input[pos])) pos++;
    }

    // Exponent part
    if (pos < input.size() && (input[pos] == 'e' || input[pos] == 'E'))
    {
      pos++;
      if (pos < input.size() && (input[pos] == '+' || input[pos] == '-'))
        pos++;
      if (pos >= input.size() || !std::isdigit(input[pos]))
        throw make_error("Expected digit in exponent");
      while (pos < input.size() && std::isdigit(input[pos])) pos++;
    }

    // Convert the substring to a double using from_chars (C++17)
    std::string_view num_str = input.substr(start, pos - start);
    double result = 0.0;
    try
    {
      result = std::stod(std::string(num_str));
    }
    catch (...)
    {
      throw make_error(std::format("Invalid number: {}", num_str));
    }
    return result;
  }

  bool parser::parse_boolean()
  {
    if (pos + 3 < input.size() && input.substr(pos, 4) == "true")
    {
      pos += 4;
      return true;
    }
    else if (pos + 4 < input.size() && input.substr(pos, 5) == "false")
    {
      pos += 5;
      return false;
    }
    else
    {
      throw make_error("Expected boolean");
    }
  }

  void parser::parse_null()
  {
    if (pos + 3 < input.size() && input.substr(pos, 4) == "null")
      pos += 4;
    else
      throw make_error("Expected null");
  }

  value::array_type parser::parse_array()
  {
    if (input[pos] != '[')
      throw make_error("Expected array");
    pos++;  // Skip opening bracket
    skip_whitespace();

    value::array_type result;

    // Handle empty array
    if (pos < input.size() && input[pos] == ']')
    {
      pos++;  // Skip closing bracket
      return result;
    }

    // Parse elements
    while (true)
    {
      result.push_back(parse_value());
      skip_whitespace();

      if (pos >= input.size())
        throw make_error("Unterminated array");

      if (input[pos] == ']')
      {
        pos++;  // Skip closing bracket
        break;
      }

      if (input[pos] != ',')
        throw make_error("Expected ',' in array");
      pos++;  // Skip comma
      skip_whitespace();
    }

    return result;
  }

  value::object_type parser::parse_object()
  {
    if (input[pos] != '{')
      throw make_error("Expected object");
    pos++;  // Skip opening brace
    skip_whitespace();

    value::object_type result;

    // Handle empty object
    if (pos < input.size() && input[pos] == '}')
    {
      pos++;  // Skip closing brace
      return result;
    }

    // Parse key-value pairs
    while (true)
    {
      // Parse key (must be a string)
      skip_whitespace();
      if (pos >= input.size() || input[pos] != '"')
        throw make_error("Expected string key in object");
      std::string key = parse_string();

      // Parse colon
      skip_whitespace();
      if (pos >= input.size() || input[pos] != ':')
        throw make_error("Expected ':' in object");
      pos++;  // Skip colon

      // Parse value
      skip_whitespace();
      result[key] = parse_value();
      skip_whitespace();

      if (pos >= input.size())
        throw make_error("Unterminated object");

      if (input[pos] == '}')
      {
        pos++;  // Skip closing brace
        break;
      }

      if (input[pos] != ',')
        throw make_error("Expected ',' in object");
      pos++;  // Skip comma
    }

    return result;
  }

  value parser::parse_value()
  {
    skip_whitespace();

    if (is_end())
      throw make_error("Unexpected end of input");

    switch (input[pos])
    {
      case '{':
        return value(parse_object());
      case '[':
        return value(parse_array());
      case '"':
        return value(parse_string());
      case 't':
      case 'f':
        return value(parse_boolean());
      case 'n':
        parse_null();
        return value();
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return value(parse_number());
      default:
        throw make_error(std::format("Unexpected character '{}'", input[pos]));
    }
  }

  parser::parser(std::string_view json_str) : input(json_str) {}

  value parser::parse()
  {
    skip_whitespace();
    value result = parse_value();
    skip_whitespace();

    if (!is_end())
      throw make_error("Expected end of input");

    return result;
  }

  std::expected<value, parse_error> parser::try_parse(std::string_view json_str, std::string filename) noexcept
  {
    try
    {
      parser p(json_str);
      p.filename = filename;
      return p.parse();
    }
    catch (const parse_error &e)
    {
      return std::unexpected(e);
    }
  }

  value parse(std::string_view json_str)
  {
    parser p(json_str);
    return p.parse();
  }

  std::expected<value, parse_error> try_parse(std::string_view json_str) noexcept { return parser::try_parse(json_str); }

  // Pretty printer for JSON values
  std::string pretty_printer::indent(int level) const { return std::string(level * indent_size, ' '); }

  std::string pretty_printer::print_internal(const value &v, int level) const
  {
    switch (v.type())
    {
      case Value_type::null:
        return "null";

      case Value_type::boolean:
        return v.as_boolean() ? "true" : "false";

      case Value_type::number:
        return std::format("{}", v.as_number());

      case Value_type::string:
        return std::format("\"{}\"", escape_string(v.as_string()));

      case Value_type::array:
      {
        const auto &arr = v.as_array();
        if (arr.empty())
          return "[]";

        std::string result = "[\n";
        for (size_t i = 0; i < arr.size(); ++i)
        {
          result += indent(level + 1) + print_internal(arr[i], level + 1);
          if (i < arr.size() - 1)
            result += ",";
          result += "\n";
        }
        result += indent(level) + "]";
        return result;
      }

      case Value_type::object:
      {
        const auto &obj = v.as_object();
        if (obj.empty())
          return "{}";

        std::string result = "{\n";
        size_t i = 0;
        for (const auto &[key, value] : obj)
        {
          result += indent(level + 1) + std::format("\"{}\": {}", key, print_internal(value, level + 1));
          if (i < obj.size() - 1)
            result += ",";
          result += "\n";
          i++;
        }
        result += indent(level) + "}";
        return result;
      }

      default:
        return "";  // Should never happen
    }
  }

  std::string pretty_printer::escape_string(const std::string &s) const
  {
    std::string result;
    result.reserve(s.size());

    for (char c : s)
    {
      switch (c)
      {
        case '\"':
          result += "\\\"";
          break;
        case '\\':
          result += "\\\\";
          break;
        case '\b':
          result += "\\b";
          break;
        case '\f':
          result += "\\f";
          break;
        case '\n':
          result += "\\n";
          break;
        case '\r':
          result += "\\r";
          break;
        case '\t':
          result += "\\t";
          break;
        default:
          if (static_cast<unsigned char>(c) < 32)
            result += std::format("\\u{:04x}", static_cast<unsigned int>(c));
          else
            result.push_back(c);
      }
    }

    return result;
  }

  pretty_printer::pretty_printer(const value &v, int indent) : val(v), indent_size(indent) {}

  std::string pretty_printer::to_string() const { return print_internal(val, 0); }

  std::string to_string(const value &v)
  {
    pretty_printer printer(v, 0);  // No indentation for compact output
    return printer.to_string();
  }

  std::string pretty_print(const value &v, int indent)
  {
    pretty_printer printer(v, indent);
    return printer.to_string();
  }

  using array_type = std::vector<value>;
  using object_type = std::map<std::string, value>;
}  // namespace jsn

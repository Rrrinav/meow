#pragma once

/* A very simple JSON parser and serializer
 * Author: Rinav (rrrinav)
 *
 * NOTE: This is a simple JSON parser and serializer written in C++. This is for my own use and is not intended for production use.
 *  only supports: null, Boolean, Number, String, Array, Object and doesn't support many edge cases that I haven't found yet.
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <cctype>
#include <expected>
#include <format>
#include <source_location>
#include <print>

namespace jsn
{
  enum class Value_type
  {
    null,
    boolean,
    number,
    string,
    array,
    object
  };

  // JSON value class using std::variant
  class value
  {
  public:
    using array_type = std::vector<value>;
    using object_type = std::map<std::string, value>;

  private:
    using json_variant = std::variant<std::monostate,  // > Represents null
                                      bool,            // > Boolean
                                      double,          // > Number
                                      std::string,     // > String
                                      array_type,      // > Array
                                      object_type      // > Object
                                      >;

    json_variant data;

    // Helper to get the value_type from the variant index
    [[nodiscard]] Value_type get_type_from_index() const
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

    [[nodiscard]] std::string_view type_to_string(Value_type type) const
    {
      switch (type) {
        case Value_type::null:    return "null";
        case Value_type::boolean: return "boolean";
        case Value_type::number:  return "number";
        case Value_type::string:  return "string";
        case Value_type::array:   return "array";
        case Value_type::object:  return "object";
        default:                  return "unknown";
      }
    }
  public:
    // Constructors
    value() noexcept : data(std::monostate{}) {}
    value(std::nullptr_t) noexcept : data(std::monostate{}) {}
    value(bool val) noexcept : data(val) {}

    // Numeric constructors with automatic conversion to double
    template <std::floating_point T>
    value(T val) noexcept : data(static_cast<double>(val))
    {
    }

    template <std::integral T>
    value(T val) noexcept : data(static_cast<double>(val))
    {
    }

    // String constructors
    value(const char *val) : data(std::string(val)) {}
    value(std::string_view val) : data(std::string(val)) {}
    value(const std::string &val) : data(val) {}
    value(std::string &&val) noexcept : data(std::move(val)) {}

    // Container constructors
    value(const array_type &val) : data(val) {}
    value(array_type &&val) noexcept : data(std::move(val)) {}
    value(const object_type &val) : data(val) {}
    value(object_type &&val) noexcept : data(std::move(val)) {}

    // Type checking methods
    [[nodiscard]] Value_type type() const noexcept { return get_type_from_index(); }
    [[nodiscard]] bool is_null() const noexcept    { return std::holds_alternative<std::monostate>(data); }
    [[nodiscard]] bool is_boolean() const noexcept { return std::holds_alternative<bool>(data); }
    [[nodiscard]] bool is_number() const noexcept  { return std::holds_alternative<double>(data); }
    [[nodiscard]] bool is_string() const noexcept  { return std::holds_alternative<std::string>(data); }
    [[nodiscard]] bool is_array() const noexcept   { return std::holds_alternative<array_type>(data); }
    [[nodiscard]] bool is_object() const noexcept  { return std::holds_alternative<object_type>(data); }


    // Value access methods with type checking
    [[nodiscard]] bool as_boolean() const
    {
      if (!is_boolean())
        throw std::runtime_error(std::format("Type error: expected boolean, got {}", type_to_string(type())));

      return std::get<bool>(data);
    }

    [[nodiscard]] double as_number() const
    {
      if (!is_number())
        throw std::runtime_error(std::format("Type error: expected number, got {}", type_to_string(type())));

      return std::get<double>(data);
    }

    [[nodiscard]] const std::string &as_string() const
    {
      if (!is_string()) 
        throw std::runtime_error(std::format("Type error: expected string, got {}", type_to_string(type())));

      return std::get<std::string>(data);
    }

    [[nodiscard]] const array_type &as_array() const
    {
      if (!is_array())
        throw std::runtime_error(std::format("Type error: expected array, got {}", type_to_string(type())));

      return std::get<array_type>(data);
    }

    [[nodiscard]] const object_type &as_object() const
    {
      if (!is_object())
        throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

      return std::get<object_type>(data);
    }

    // Mutable access
    [[nodiscard]] std::string &as_string()
    {
      if (!is_string())
        throw std::runtime_error(std::format("Type error: expected string, got {}", type_to_string(type())));

      return std::get<std::string>(data);
    }

    [[nodiscard]] array_type &as_array()
    {
      if (!is_array())
        throw std::runtime_error(std::format("Type error: expected array, got {}", type_to_string(type())));

      return std::get<array_type>(data);
    }

    [[nodiscard]] object_type &as_object()
    {
      if (!is_object())
        throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

      return std::get<object_type>(data);
    }

    // Array element access
    [[nodiscard]] const value operator[](size_t index) const
    {
      if (!is_array())
        throw std::runtime_error(std::format("Type error: expected array, got {}", type_to_string(type())));

      const auto &arr = std::get<array_type>(data);

      if (index >= arr.size())
        return value();

      return arr[index];
    }

    // Object element access
    [[nodiscard]] const value operator[](std::string_view key) const
    {
      if (!is_object()) throw std::runtime_error(std::format("Type error: expected object, got {}", type_to_string(type())));

      const auto &obj = std::get<object_type>(data);
      auto it = obj.find(std::string(key));
      if (it == obj.end()) return value();

      return it->second;
    }

    // Safe access with std::expected (C++23)
    [[nodiscard]] std::expected<bool, std::string> try_as_boolean() const noexcept
    {
      if (!is_boolean())
        return std::unexpected(std::format("Type error: expected boolean, got {}", type_to_string(type())));

      return std::get<bool>(data);
    }

    [[nodiscard]] std::expected<double, std::string> try_as_number() const noexcept
    {
      if (!is_number())
        return std::unexpected(std::format("Type error: expected number, got {}", type_to_string(type())));

      return std::get<double>(data);
    }

    [[nodiscard]] std::expected<std::string, std::string> try_as_string() const noexcept
    {
      if (!is_string())
        return std::unexpected(std::format("Type error: expected string, got {}", type_to_string(type())));

      return std::string(std::get<std::string>(data));
    }

    [[nodiscard]] std::expected<array_type, std::string> try_as_array() const noexcept
    {
      if (!is_array())
        return std::unexpected(std::format("Type error: expected array, got {}", type_to_string(type())));

      return std::get<array_type>(data);
    }

    [[nodiscard]] const std::expected<object_type, std::string> try_as_object() const noexcept
    {
      if (!is_object())
        return std::unexpected(std::format("Type error: expected object, got {}", type_to_string(type())));

      return std::get<object_type>(data);
    }

    // Optional access versions
    [[nodiscard]] std::optional<bool> boolean_value() const noexcept
    {
      if (!is_boolean())
        return std::nullopt;

      return std::get<bool>(data);
    }

    [[nodiscard]] std::optional<double> number_value() const noexcept
    {
      if (!is_number())
        return std::nullopt;

      return std::get<double>(data);
    }

    [[nodiscard]] std::optional<std::string> string_value() const noexcept
    {
      if (!is_string())
        return std::nullopt;

      return std::string(std::get<std::string>(data));
    }

    [[nodiscard]] std::optional<array_type> array_value() const noexcept
    {
      if (!is_array())
        return std::nullopt;

      return std::get<array_type>(data);
    }

    [[nodiscard]] std::optional<object_type> object_value() const noexcept
    {
      if (!is_object())
        return std::nullopt;

      return std::get<object_type>(data);
    }
  };

  struct json_location
  {
    std::size_t position = 0;
    std::size_t line     = 1;
    std::size_t column   = 1;
    std::string filename = "<unknown>";
  };

  inline json_location compute_location(std::string_view input, size_t pos, const std::string &filename = "<unknown>")
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
  // Parse error with contextual information
  struct parse_error : public std::runtime_error
  {
    std::string message;
    json_location location;
    std::string context;

    parse_error(const std::string &message, const json_location &loc, std::string_view ctx)
        : std::runtime_error(std::format("{}:{}:{}: {} (context: '{}')", loc.filename, loc.line, loc.column, message, ctx)),
          message(message),
          location(loc),
          context(ctx)
    {
    }
  };

  // JSON Parser class
  class parser
  {
  private:
    std::string_view input;
    size_t pos = 0;
  public:
    std::string filename = "<unknown>";
  private:
    // Helper methods to skip whitespace and check if we're at the end
    void skip_whitespace() noexcept
    {
      while (pos < input.size() && std::isspace(input[pos])) pos++;
    }

    [[nodiscard]] bool is_end() const noexcept { return pos >= input.size(); }

    // Get context around the current position for error messages
    [[nodiscard]] std::string get_context(size_t length = 10) const noexcept
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
        if (std::isalnum(input[i])) break;
        if (input[i] == '\r' || input[i] == '\t' || std::isspace(input[i])) continue;
        begin_pos = i;
        break;
      }
      return std::string(input.substr(begin_pos, end_pos - begin_pos));
    }

    [[nodiscard]] parse_error make_error(const std::string &message) const
    {
      json_location loc = compute_location(input, pos, filename);
      return parse_error(message, loc, get_context());
    }

    [[nodiscard]] std::string parse_string()
    {
      if (input[pos] != '"') throw make_error("Expected string");
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
              if (pos + 4 >= input.size()) throw make_error("Incomplete Unicode escape sequence");
              // Convert hex digits to code point
              std::string hex = std::string(input.substr(pos + 1, 4));
              pos += 4;  // Skip the 4 hex digits
              // Basic implementation - would need more for full Unicode support
              char16_t code_point = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
              // This is simplified UTF-16 to UTF-8 conversion
              if (code_point < 0x80) { result.push_back(static_cast<char>(code_point)); }
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
        else { result.push_back(input[pos]); }
        pos++;
      }

      if (pos >= input.size() || input[pos] != '"') throw make_error("Unterminated string");
      pos++;  // Skip closing quote

      return result;
    }

    // Parse a JSON number
    [[nodiscard]] double parse_number()
    {
      size_t start = pos;

      // Handle negative numbers
      if (input[pos] == '-') pos++;

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
        if (pos >= input.size() || !std::isdigit(input[pos])) throw make_error("Expected digit after decimal point");
        while (pos < input.size() && std::isdigit(input[pos])) pos++;
      }

      // Exponent part
      if (pos < input.size() && (input[pos] == 'e' || input[pos] == 'E'))
      {
        pos++;
        if (pos < input.size() && (input[pos] == '+' || input[pos] == '-')) pos++;
        if (pos >= input.size() || !std::isdigit(input[pos])) throw make_error("Expected digit in exponent");
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

    // Parse a JSON boolean
    [[nodiscard]] bool parse_boolean()
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
      else { throw make_error("Expected boolean"); }
    }

    // Parse a JSON null
    void parse_null()
    {
      if (pos + 3 < input.size() && input.substr(pos, 4) == "null")
        pos += 4;
      else
        throw make_error("Expected null");
    }

    // Parse a JSON array
    [[nodiscard]] value::array_type parse_array()
    {
      if (input[pos] != '[') throw make_error("Expected array");
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

        if (pos >= input.size()) throw make_error("Unterminated array");

        if (input[pos] == ']')
        {
          pos++;  // Skip closing bracket
          break;
        }

        if (input[pos] != ',') throw make_error("Expected ',' in array");
        pos++;  // Skip comma
        skip_whitespace();
      }

      return result;
    }

    // Parse a JSON object
    [[nodiscard]] value::object_type parse_object()
    {
      if (input[pos] != '{') throw make_error("Expected object");
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
        if (pos >= input.size() || input[pos] != '"') throw make_error("Expected string key in object");
        std::string key = parse_string();

        // Parse colon
        skip_whitespace();
        if (pos >= input.size() || input[pos] != ':') throw make_error("Expected ':' in object");
        pos++;  // Skip colon

        // Parse value
        skip_whitespace();
        result[key] = parse_value();
        skip_whitespace();

        if (pos >= input.size()) throw make_error("Unterminated object");

        if (input[pos] == '}')
        {
          pos++;  // Skip closing brace
          break;
        }

        if (input[pos] != ',') throw make_error("Expected ',' in object");
        pos++;  // Skip comma
      }

      return result;
    }

    // Parse any JSON value
    [[nodiscard]] value parse_value()
    {
      skip_whitespace();

      if (is_end()) throw make_error("Unexpected end of input");

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

  public:
    explicit parser(std::string_view json_str) : input(json_str) {}

    [[nodiscard]] value parse()
    {
      skip_whitespace();
      value result = parse_value();
      skip_whitespace();

      if (!is_end()) throw make_error("Expected end of input");

      return result;
    }

    // Static convenience method
    [[nodiscard]] static std::expected<value, parse_error> try_parse(std::string_view json_str, std::string filename = "<config file>") noexcept
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
  };

  // Helper functions for parsing JSON
  [[nodiscard]] value parse(std::string_view json_str)
  {
    parser p(json_str);
    return p.parse();
  }

  [[nodiscard]] std::expected<value, parse_error> try_parse(std::string_view json_str) noexcept { return parser::try_parse(json_str); }

  // Pretty printer for JSON values
  class pretty_printer
  {
  private:
    const value &val;
    int indent_size;

    [[nodiscard]] std::string indent(int level) const { return std::string(level * indent_size, ' '); }

    [[nodiscard]] std::string print_internal(const value &v, int level) const
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
          if (arr.empty()) return "[]";

          std::string result = "[\n";
          for (size_t i = 0; i < arr.size(); ++i)
          {
            result += indent(level + 1) + print_internal(arr[i], level + 1);
            if (i < arr.size() - 1) result += ",";
            result += "\n";
          }
          result += indent(level) + "]";
          return result;
        }

        case Value_type::object:
        {
          const auto &obj = v.as_object();
          if (obj.empty()) return "{}";

          std::string result = "{\n";
          size_t i = 0;
          for (const auto &[key, value] : obj)
          {
            result += indent(level + 1) + std::format("\"{}\": {}", key, print_internal(value, level + 1));
            if (i < obj.size() - 1) result += ",";
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

    [[nodiscard]] std::string escape_string(const std::string &s) const
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

  public:
    explicit pretty_printer(const value &v, int indent = 2) : val(v), indent_size(indent) {}

    [[nodiscard]] std::string to_string() const { return print_internal(val, 0); }
  };

  // String conversion utilities
  [[nodiscard]] std::string to_string(const value &v)
  {
    pretty_printer printer(v, 0);  // No indentation for compact output
    return printer.to_string();
  }

  [[nodiscard]] std::string pretty_print(const value &v, int indent = 2)
  {
    pretty_printer printer(v, indent);
    return printer.to_string();
  }
}  // namespace jsn

#pragma once

/* A very simple JSON parser and serializer
 * Author: Rinav (rrrinav)
 *
 * NOTE: This is a simple JSON parser and serializer written in C++. This is for my own use and is not intended for production use.
 *  only supports: null, Boolean, Number, String, Array, Object and doesn't support many edge cases that I haven't found yet.
 *  This is very particular to this tool and is not intended to be a general purpose JSON parser.
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <expected>
#include <optional>

namespace jsn
{
  enum class Value_type { null, boolean, number, string, array, object };

  class value
  {
  public:
    using array_type  = std::vector<value>;
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
    [[nodiscard]] Value_type get_type_from_index() const;
    [[nodiscard]] std::string_view type_to_string(Value_type type) const;

  public:
    // Constructors
    value() noexcept;
    value(std::nullptr_t) noexcept;
    value(bool val) noexcept;

    template <std::floating_point T>
    value(T val) noexcept : data(static_cast<double>(val)) {}

    template <std::integral T> value(T val) noexcept : data(static_cast<double>(val)) {}

    // String constructors
    value(const char *val);
    value(std::string_view val);
    value(const std::string &val);
    value(std::string &&val) noexcept;

    // Container constructors
    value(const array_type &val);
    value(array_type &&val) noexcept;
    value(const object_type &val);
    value(object_type &&val) noexcept;

    [[nodiscard]] Value_type type() const noexcept;
    [[nodiscard]] bool is_null() const noexcept;
    [[nodiscard]] bool is_boolean() const noexcept;
    [[nodiscard]] bool is_number() const noexcept;
    [[nodiscard]] bool is_string() const noexcept;
    [[nodiscard]] bool is_array() const noexcept;
    [[nodiscard]] bool is_object() const noexcept;

    template <typename T>
    T value_or(const T &fallback) const
    {
      if constexpr (std::is_same_v<T, bool>)
        return boolean_opt().value_or(fallback);
      else if constexpr (std::is_same_v<T, double>)
        return number_opt().value_or(fallback);
      else if constexpr (std::is_same_v<T, std::string>)
        return string_opt().value_or(fallback);
      else if constexpr (std::is_same_v<T, array_type>)
        return array_opt().value_or(fallback);
      else if constexpr (std::is_same_v<T, object_type>)
        return object_opt().value_or(fallback);
      else
        static_assert(!sizeof(T), "Unsupported type for value_or()");
    }

    operator bool() const;
    operator double() const;
    operator std::string() const;
    operator int() const;
    operator array_type() const;
    operator object_type() const;

    [[nodiscard]] bool as_boolean() const;
    [[nodiscard]] int as_int() const;
    [[nodiscard]] double as_number() const;
    [[nodiscard]] const std::string &as_string() const;
    [[nodiscard]] const array_type &as_array() const;
    [[nodiscard]] const object_type &as_object() const;

    [[nodiscard]] const value operator[](const std::size_t index) const;
    [[nodiscard]] const value operator[](int index) const;
    [[nodiscard]] const value operator[](const char *key) const;
    [[nodiscard]] const value operator[](const std::string &key) const;

    [[nodiscard]] std::expected<bool, std::string> expect_boolean() const noexcept;
    [[nodiscard]] std::expected<double, std::string> expect_number() const noexcept;
    [[nodiscard]] std::expected<std::string, std::string> expect_string() const noexcept;
    [[nodiscard]] std::expected<array_type, std::string> expect_array() const noexcept;
    [[nodiscard]] std::expected<object_type, std::string> expect_object() const noexcept;

    [[nodiscard]] std::optional<bool> boolean_opt() const noexcept;
    [[nodiscard]] std::optional<double> number_opt() const noexcept;
    [[nodiscard]] std::optional<std::string> string_opt() const noexcept;
    [[nodiscard]] std::optional<array_type> array_opt() const noexcept;
    [[nodiscard]] std::optional<object_type> object_opt() const noexcept;

    object_type& as_object();
    void set(const std::string &key, const value &val);
    void set_nested(const std::string &path, const value &val);
    std::expected<size_t, std::string> push(std::string path, const value &val);
    std::expected<size_t, std::string> put_at(std::string path, const value &val);
  };

  using array_type = std::vector<value>;
  using object_type = std::map<std::string, value>;

  struct json_location
  {
    std::size_t position = 0;
    std::size_t line = 1;
    std::size_t column = 1;
    std::string filename = "<unknown>";
  };

  json_location compute_location(std::string_view input, size_t pos, const std::string &filename = "<unknown>");

  struct parse_error : public std::runtime_error
  {
    std::string message;
    json_location location;
    std::string context;

    parse_error(const std::string &message, const json_location &loc, std::string_view ctx);
  };

  class parser
  {
  private:
    std::string_view input;
    size_t pos = 0;

  public:
    std::string filename = "<unknown>";

  private:
    // Helper methods to skip whitespace and check if we're at the end
    void skip_whitespace() noexcept;
    [[nodiscard]] bool is_end() const noexcept;

    // Get context around the current position for error messages
    [[nodiscard]] std::string get_context(size_t length = 10) const noexcept;
    [[nodiscard]] parse_error make_error(const std::string &message) const;

    [[nodiscard]] std::string parse_string();
    [[nodiscard]] double parse_number();
    [[nodiscard]] bool parse_boolean();
    void parse_null();
    [[nodiscard]] value::array_type parse_array();
    [[nodiscard]] value::object_type parse_object();
    [[nodiscard]] value parse_value();

  public:
    explicit parser(std::string_view json_str);
    [[nodiscard]] value parse();

    [[nodiscard]] static std::expected<value, parse_error> try_parse(std::string_view json_str,
                                                                    std::string filename = "<config file>") noexcept;
  };

  [[nodiscard]] value parse(std::string_view json_str);
  [[nodiscard]] std::expected<value, parse_error> try_parse(std::string_view json_str) noexcept;

  class pretty_printer
  {
  private:
    const value &val;
    int indent_size;

    [[nodiscard]] std::string indent(int level) const;
    [[nodiscard]] std::string print_internal(const value &v, int level) const;
    [[nodiscard]] std::string escape_string(const std::string &s) const;

  public:
    explicit pretty_printer(const value &v, int indent = 2);
    [[nodiscard]] std::string to_string() const;
  };

  [[nodiscard]] std::string to_string(const value &v);
  [[nodiscard]] std::string pretty_print(const value &v, int indent = 2);
}  // namespace jsn

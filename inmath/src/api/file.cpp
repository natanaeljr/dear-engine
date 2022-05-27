#include "./file.hpp"

#include <fstream>
#include <optional>
#include <string>

#include "./log.hpp"

/// Read file contents to a string
auto read_file_to_string(const std::string& filename) -> std::optional<std::string>
{
  std::string string;
  std::fstream fstream(filename, std::ios::in | std::ios::binary);
  if (!fstream) { ERROR("{} ({})", std::strerror(errno), filename); return std::nullopt; }
  fstream.seekg(0, std::ios::end);
  string.reserve(fstream.tellg());
  fstream.seekg(0, std::ios::beg);
  string.assign((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
  return string;
}

/* cirsat: Bench Reader for k-LUT Networks */
#pragma once
#include "lut_network.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace cirsat {

template <typename Ntk>
class bench_reader
{
public:
  explicit bench_reader(Ntk& ntk) : ntk_(ntk) {}

  bool parse(std::istream& is)
  {
    const std::string flag_input = "INPUT";
    const std::string flag_output = "OUTPUT";
    const std::string flag_lut = "LUT";
    const std::string flag_gnd = "gnd";
    
    std::string line;
    while (std::getline(is, line))
    {
      line = trim(line);
      if (line.empty() || line[0] == '#')
        continue;

      if (line.find(flag_input) != std::string::npos)
      {
        if (!parse_input(line))
          return false;
      }
      else if (line.find(flag_output) != std::string::npos)
      {
        if (!parse_output(line))
          return false;
      }
      else if (line.find(flag_gnd) != std::string::npos)
      {
        if (!parse_gnd(line))
          return false;
      }
      else if (line.find(flag_lut) != std::string::npos)
      {
        if (!parse_lut(line))
          return false;
      }
    }

    create_outputs();
    return true;
  }

  bool parse_file(const std::string& filename)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      std::cerr << "Error: Cannot open file " << filename << std::endl;
      return false;
    }
    
    bool result = parse(file);
    file.close();
    return result;
  }

private:
  bool parse_input(const std::string& line)
  {
    // INPUT(name) -> split by "( )"
    std::vector<std::string> parts = str_split(line, "( )");
    if (parts.size() < 2)
      return false;
    
    std::string name = parts[1];
    auto signal = ntk_.create_pi(name);
    signal_map_[name] = signal;
    return true;
  }

  bool parse_output(const std::string& line)
  {
    // OUTPUT(name) -> split by "( )"
    std::vector<std::string> parts = str_split(line, "( )");
    if (parts.size() < 2)
      return false;
    
    std::string name = parts[1];
    output_names_.push_back(name);
    return true;
  }

  bool parse_gnd(const std::string& line)
  {
    // n0 = gnd -> split by "= "
    std::vector<std::string> parts = str_split(line, "= ");
    if (parts.size() < 1)
      return false;
    
    std::string gnd_name = parts[0];
    
    // Create a constant 0 signal (ground)
    auto gnd_signal = ntk_.get_constant(false);
    signal_map_[gnd_name] = gnd_signal;
    
    return true;
  }

  bool parse_lut(const std::string& line)
  {
    // output = LUT 0xhex (input1, input2, ...) -> split by ",=( )"
    std::vector<std::string> parts = str_split(line, ",=( )");
    if (parts.size() < 3)
      return false;
    
    std::string output_name = parts[0];
    std::string tt = parts[2];
    std::vector<std::string> input_names(parts.begin() + 3, parts.end());

    // Add "0x" prefix if not present
    if (tt.find("0x") == std::string::npos && tt.find("0X") == std::string::npos)
      tt = "0x" + tt;

    std::vector<typename Ntk::signal> inputs;
    for (const auto& name : input_names)
    {
      if (signal_map_.find(name) == signal_map_.end())
      {
        std::cerr << "Error: Undefined signal " << name << " in gate " << output_name << std::endl;
        return false;
      }
      inputs.push_back(signal_map_[name]);
    }

    auto output_signal = ntk_.create_node(tt, inputs, output_name);
    signal_map_[output_name] = output_signal;

    return true;
  }

  void create_outputs()
  {
    for (const auto& name : output_names_)
    {
      if (signal_map_.find(name) != signal_map_.end())
      {
        ntk_.create_po(signal_map_[name], name);
      }
      else
      {
        std::cerr << "Warning: Output " << name << " not defined" << std::endl;
      }
    }
  }

  std::vector<std::string> str_split(const std::string& str, const std::string& delimiters)
  {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);
    
    while (end != std::string::npos)
    {
      if (end > start)
      {
        std::string token = str.substr(start, end - start);
        // Trim whitespace
        size_t first = token.find_first_not_of(" \t\r\n");
        if (first != std::string::npos)
        {
          size_t last = token.find_last_not_of(" \t\r\n");
          result.push_back(token.substr(first, last - first + 1));
        }
      }
      start = end + 1;
      end = str.find_first_of(delimiters, start);
    }
    
    if (start < str.length())
    {
      std::string token = str.substr(start);
      size_t first = token.find_first_not_of(" \t\r\n");
      if (first != std::string::npos)
      {
        size_t last = token.find_last_not_of(" \t\r\n");
        result.push_back(token.substr(first, last - first + 1));
      }
    }
    
    return result;
  }

  std::string trim(const std::string& str)
  {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
      return "";
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
  }

private:
  Ntk& ntk_;
  std::unordered_map<std::string, typename Ntk::signal> signal_map_;
  std::vector<std::string> output_names_;
};

} // namespace cirsat

/* cirsat: Circuit SAT Solver based on DPLL */
#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <cassert>
#include <cstdint>

namespace cirsat {

class lut_network
{
public:
  using signal = uint32_t;
  using node = uint32_t;

  struct lut_node {
    std::string name;
    std::string truth_table;
    std::vector<signal> fanins;
    bool is_pi = false;
    bool is_po = false;
  };

  struct po_info {
    signal sig;
    std::string name;
  };

  lut_network()
  {
    nodes_.emplace_back();
    nodes_[0].name = "const0";
    nodes_[0].truth_table = "0x0";
    nodes_[0].is_pi = false;
  }

  signal create_pi(const std::string& name = "")
  {
    node n = nodes_.size();
    nodes_.emplace_back();
    nodes_[n].name = name.empty() ? ("pi" + std::to_string(num_pis_)) : name;
    nodes_[n].is_pi = true;
    nodes_[n].truth_table = "0x2";
    pis_.push_back(n);
    num_pis_++;
    return make_signal(n, false);
  }

  uint32_t create_po(signal sig, const std::string& name = "")
  {
    pos_.push_back({sig, name.empty() ? ("po" + std::to_string(num_pos_)) : name});
    num_pos_++;
    return num_pos_ - 1;
  }

  signal create_node(const std::string& tt, const std::vector<signal>& fanins, 
                     const std::string& name = "")
  {
    node n = nodes_.size();
    nodes_.emplace_back();
    nodes_[n].name = name.empty() ? ("n" + std::to_string(n)) : name;
    nodes_[n].truth_table = tt;
    nodes_[n].fanins = fanins;
    nodes_[n].is_pi = false;
    num_gates_++;
    return make_signal(n, false);
  }

  signal get_constant(bool value) const { return make_signal(0, value); }
  uint32_t num_pis() const { return num_pis_; }
  uint32_t num_pos() const { return num_pos_; }
  uint32_t num_gates() const { return num_gates_; }
  uint32_t size() const { return nodes_.size(); }
  node get_node(signal s) const { return s >> 1; }
  bool is_complemented(signal s) const { return (s & 1) == 1; }
  static signal make_signal(node n, bool complement = false) { return (n << 1) | (complement ? 1 : 0); }
  const lut_node& get_node_info(node n) const { assert(n < nodes_.size()); return nodes_[n]; }
  const std::vector<node>& get_pis() const { return pis_; }
  const std::vector<po_info>& get_pos() const { return pos_; }
  const std::vector<lut_node>& get_nodes() const { return nodes_; }
  bool is_pi(node n) const { return n < nodes_.size() && nodes_[n].is_pi; }
  bool is_constant(node n) const { return n == 0; }

  void print_stats() const
  {
    std::cout << "LUT Network: PIs=" << num_pis_ << ", POs=" << num_pos_ 
              << ", LUTs=" << num_gates_ << ", Nodes=" << nodes_.size() << std::endl;
  }

  void print_network() const
  {
    std::cout << "\n=== LUT Network ===" << std::endl;
    std::cout << "Inputs: ";
    for (auto pi : pis_) std::cout << nodes_[pi].name << " ";
    std::cout << "\n\nGates:" << std::endl;
    for (size_t i = 1; i < nodes_.size(); ++i)
    {
      if (!nodes_[i].is_pi && !is_constant(i))
      {
        std::cout << "  " << nodes_[i].name << " = LUT " << nodes_[i].truth_table << " ( ";
        for (size_t j = 0; j < nodes_[i].fanins.size(); ++j)
        {
          auto fn = get_node(nodes_[i].fanins[j]);
          auto ic = is_complemented(nodes_[i].fanins[j]);
          std::cout << (ic ? "!" : "") << nodes_[fn].name;
          if (j < nodes_[i].fanins.size() - 1) std::cout << ", ";
        }
        std::cout << " )" << std::endl;
      }
    }
    std::cout << "\nOutputs:" << std::endl;
    for (const auto& po : pos_)
    {
      auto pn = get_node(po.sig);
      auto pc = is_complemented(po.sig);
      std::cout << "  " << po.name << " = " << (pc ? "!" : "") << nodes_[pn].name << std::endl;
    }
  }

  // === Network Analysis Functions ===

  void compute_levels()
  {
    levels_.clear();
    levels_.resize(nodes_.size(), -1);
    levels_[0] = 0; // constant node
    
    for (auto pi : pis_)
      levels_[pi] = 0;
    
    for (const auto& po : pos_)
      compute_level_recursive(get_node(po.sig));
  }

  int get_level(node n) const
  {
    return (n < levels_.size()) ? levels_[n] : -1;
  }

  uint64_t parse_truth_table(const std::string& tt_str) const
  {
    uint64_t tt = 0;
    if (tt_str.find("0x") == 0 || tt_str.find("0X") == 0)
      tt = std::stoull(tt_str, nullptr, 16);
    else if (tt_str.find("0b") == 0 || tt_str.find("0B") == 0)
      tt = std::stoull(tt_str.substr(2), nullptr, 2);
    else
      tt = std::stoull(tt_str, nullptr, 10);
    return tt;
  }

  std::string classify_function(node n) const
  {
    const auto& info = nodes_[n];
    if (info.is_pi) return "PRIMARY_INPUT";
    if (is_constant(n)) return "CONSTANT";
    
    uint64_t tt = parse_truth_table(info.truth_table);
    unsigned num_inputs = info.fanins.size();
    if (num_inputs == 0) return "CONSTANT";
    
    unsigned entries = 1u << num_inputs;
    uint64_t full_mask = (entries >= 64) ? ~0ULL : ((1ULL << entries) - 1);
    
    if (tt == 0) return "CONST_0";
    if (tt == full_mask) return "CONST_1";
    
    // Check for buffer/inverter (projection)
    for (unsigned i = 0; i < num_inputs; ++i) {
      uint64_t proj = 0;
      for (unsigned j = 0; j < entries; ++j) {
        if ((j >> i) & 1) proj |= (1ULL << j);
      }
      if (tt == proj) return "BUFFER_" + std::to_string(i);
      if (tt == (proj ^ full_mask)) return "INVERTER_" + std::to_string(i);
    }
    
    // Check for XOR/XNOR
    uint64_t xor_mask = 0;
    for (unsigned i = 0; i < entries; ++i) {
      if (__builtin_popcount(i) & 1) xor_mask |= (1ULL << i);
    }
    if (tt == xor_mask) return "XOR";
    if (tt == (xor_mask ^ full_mask)) return "XNOR";
    
    // 2-input gate classification
    if (num_inputs == 2) {
      switch (tt & 0xF) {
        case 0x1: return "NOR";
        case 0x2: return "A_AND_NOT_B";
        case 0x4: return "NOT_A_AND_B";
        case 0x6: return "XOR";
        case 0x7: return "NAND";
        case 0x8: return "AND";
        case 0x9: return "XNOR";
        case 0xE: return "OR";
        default: break;
      }
    }
    
    return "GENERIC_LUT";
  }

  void print_detailed_analysis() const
  {
    std::cout << "\n=== Detailed Network Analysis ===" << std::endl;
    std::cout << "Total nodes: " << nodes_.size() << std::endl;
    std::cout << "Primary inputs: " << num_pis_ << std::endl;
    std::cout << "Primary outputs: " << num_pos_ << std::endl;
    std::cout << "LUT gates: " << num_gates_ << std::endl;
    
    std::unordered_map<std::string, int> type_count;
    for (size_t i = 1; i < nodes_.size(); ++i) {
      if (!nodes_[i].is_pi && !is_constant(i)) {
        std::string type = classify_function(i);
        type_count[type]++;
      }
    }
    
    std::cout << "\nGate Type Distribution:" << std::endl;
    for (const auto& [type, count] : type_count) {
      std::cout << "  " << type << ": " << count << std::endl;
    }
    
    std::cout << "\n=== Node Details ===" << std::endl;
    for (size_t i = 0; i < nodes_.size(); ++i) {
      const auto& node = nodes_[i];
      std::cout << "\nNode [" << i << "] " << node.name;
      if (node.is_pi) std::cout << " (PRIMARY INPUT)";
      if (is_constant(i)) std::cout << " (CONSTANT)";
      std::cout << std::endl;
      
      if (!node.is_pi && !is_constant(i)) {
        std::cout << "  Type: " << classify_function(i) << std::endl;
        std::cout << "  Truth Table: " << node.truth_table << std::endl;
        std::cout << "  Fanins (" << node.fanins.size() << "): ";
        for (size_t j = 0; j < node.fanins.size(); ++j) {
          auto fn = get_node(node.fanins[j]);
          auto ic = is_complemented(node.fanins[j]);
          std::cout << (ic ? "!" : "") << nodes_[fn].name;
          if (j < node.fanins.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        if (!levels_.empty() && get_level(i) >= 0) {
          std::cout << "  Level: " << get_level(i) << std::endl;
        }
        
        //print_truth_table_details(i);
      }
    }
    
    std::cout << "\n=== Primary Outputs ==="  << std::endl;
    for (const auto& po : pos_) {
      auto pn = get_node(po.sig);
      auto pc = is_complemented(po.sig);
      std::cout << "  " << po.name << " = " << (pc ? "!" : "") << nodes_[pn].name;
      if (!levels_.empty() && get_level(pn) >= 0) {
        std::cout << " (level " << get_level(pn) << ")";
      }
      std::cout << std::endl;
    }
  }

  void print_truth_table_details(node n) const
  {
    const auto& info = nodes_[n];
    if (info.is_pi || info.fanins.empty()) return;
    
    uint64_t tt = parse_truth_table(info.truth_table);
    unsigned num_inputs = info.fanins.size();
    unsigned entries = 1u << num_inputs;
    
    std::cout << "  Truth Table Mapping:" << std::endl;
    std::cout << "    ";
    for (int i = num_inputs - 1; i >= 0; --i) {
      std::cout << "i" << i << (i > 0 ? " " : "");
    }
    std::cout << " | out" << std::endl;
    std::cout << "    ";
    for (unsigned i = 0; i < num_inputs * 3 - 1; ++i) std::cout << "-";
    std::cout << "|----" << std::endl;
    
    for (unsigned i = 0; i < std::min(entries, 16u); ++i) {
      std::cout << "    ";
      for (int j = num_inputs - 1; j >= 0; --j) {
        std::cout << " " << ((i >> j) & 1) << " ";
      }
      std::cout << "| " << ((tt >> i) & 1) << std::endl;
    }
    if (entries > 16) {
      std::cout << "    ... (" << entries << " total entries, showing first 16)" << std::endl;
    }
  }

private:
  std::vector<lut_node> nodes_;
  std::vector<node> pis_;
  std::vector<po_info> pos_;
  uint32_t num_pis_ = 0;
  uint32_t num_pos_ = 0;
  uint32_t num_gates_ = 0;
  mutable std::vector<int> levels_;

  int compute_level_recursive(node n) const
  {
    if (n >= levels_.size()) {
      levels_.resize(nodes_.size(), -1);
    }
    if (levels_[n] != -1) return levels_[n];
    
    if (is_pi(n) || is_constant(n)) {
      levels_[n] = 0;
      return 0;
    }
    
    int max_level = -1;
    const auto& info = nodes_[n];
    for (auto fanin_sig : info.fanins) {
      auto fanin_node = get_node(fanin_sig);
      int level = compute_level_recursive(fanin_node);
      if (level > max_level) max_level = level;
    }
    
    levels_[n] = max_level + 1;
    return levels_[n];
  }
};

} // namespace cirsat

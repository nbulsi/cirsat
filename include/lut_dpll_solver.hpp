/* cirsat: LUT DPLL Solver */
#pragma once
#include "lut_network.hpp"
#include <vector>
#include <cstdint>
#include <iostream>
#include <cmath>

namespace cirsat {

class lut_dpll_solver
{
public:
  enum class lbool { l_false = 0, l_true = 1, l_undef = 2 };
  enum class solve_mode { NORMAL, VERBOSE, DEBUG };

  explicit lut_dpll_solver(const lut_network& ntk, solve_mode mode = solve_mode::NORMAL) 
    : ntk_(ntk), mode_(mode) { init(); }

  bool solve()
  {
    num_decisions_ = 0;
    num_conflicts_ = 0;
    num_implications_ = 0;

    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "Circuit: " << ntk_.num_pis() << " PIs, "
                << ntk_.num_pos() << " POs, "
                << ntk_.num_gates() << " gates" << std::endl;
    }

    //Set primary output constraints
    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "\n---Setting primary output constraints---" << std::endl;
    }
    
    for (const auto& po : ntk_.get_pos())
    {
      auto node = ntk_.get_node(po.sig);
      bool is_compl = ntk_.is_complemented(po.sig);
      lbool target_value = is_compl ? lbool::l_false : lbool::l_true;
      
      if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
        std::cout << "  Output " << po.name << " (node " << node << ") must be "
                  << (target_value == lbool::l_true ? "TRUE" : "FALSE") << std::endl;
      }
      
      if (get_value(node) == lbool::l_undef)
        set_value(node, target_value, 0);
      else if (get_value(node) != target_value) {
        if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
          std::cout << "  CONFLICT: Output already has conflicting value!" << std::endl;
        }
        return false;
      }
    }

    //Initial propagation
    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "\n---Performing initial constraint propagation---" << std::endl;
    }
    
    if (!propagate()) {
      if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
        std::cout << "  CONFLICT during initial propagation!" << std::endl;
      }
      return false;
    }
    
    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "  Initial propagation complete. Implications: " << num_implications_ << std::endl;
    }

    //Search for solution
    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "\n---Starting DPLL search---" << std::endl;
    }
    
    bool result = search();
    
    if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
      std::cout << "\n=== DPLL Search Complete ===" << std::endl;
      std::cout << "Result: " << (result ? "SATISFIABLE" : "UNSATISFIABLE") << std::endl;
    }
    
    return result;
  }

  void print_solution() const
  {
    std::cout << "\nSatisfying Assignment:" << std::endl;
    for (auto pi : ntk_.get_pis())
    {
      const auto& node_info = ntk_.get_node_info(pi);
      lbool val = get_value(pi);
      std::cout << "  " << node_info.name << " = " 
                << (val == lbool::l_true ? "1" : (val == lbool::l_false ? "0" : "X")) << std::endl;
    }
  }

  void print_stats() const
  {
    std::cout << "\nSolver Stats: Decisions=" << num_decisions_ 
              << ", Conflicts=" << num_conflicts_ 
              << ", Implications=" << num_implications_ << std::endl;
  }

private:
  void init()
  {
    values_.resize(ntk_.size(), lbool::l_undef);
    levels_.resize(ntk_.size(), -1);
    values_[0] = lbool::l_false;
    levels_[0] = 0;
  }

  lbool get_value(lut_network::node n) const { return values_[n]; }

  void set_value(lut_network::node n, lbool value, int level)
  {
    values_[n] = value;
    levels_[n] = level;
    trail_.push_back(n);
  }

  bool propagate()
  {
    bool changed = true;
    int iterations = 0;
    const int max_iterations = 1000;

    while (changed && iterations < max_iterations)
    {
      changed = false;
      iterations++;

      for (size_t i = 1; i < ntk_.size(); ++i)
      {
        if (ntk_.is_pi(i) || ntk_.is_constant(i))
          continue;

        if (get_value(i) != lbool::l_undef)
        {
          if (!check_gate_consistency(i))
          {
            num_conflicts_++;
            return false;
          }
        }
        else
        {
          auto deduced = deduce_gate_value(i);
          if (deduced != lbool::l_undef)
          {
            set_value(i, deduced, trail_.size());
            changed = true;
            num_implications_++;
            
            if (mode_ == solve_mode::DEBUG) {
              const auto& node_info = ntk_.get_node_info(i);
              std::cout << "    Implied: " << node_info.name << " = "
                        << (deduced == lbool::l_true ? "TRUE" : "FALSE") << std::endl;
            }
          }
        }
      }
    }

    return true;
  }

  lbool deduce_gate_value(lut_network::node n)
  {
    const auto& node_info = ntk_.get_node_info(n);
    const auto& fanins = node_info.fanins;

    std::vector<lbool> input_values;
    for (auto fanin_sig : fanins)
    {
      auto fanin_node = ntk_.get_node(fanin_sig);
      bool is_compl = ntk_.is_complemented(fanin_sig);
      lbool val = get_value(fanin_node);
      
      if (val == lbool::l_undef)
        return lbool::l_undef;
      
      if (is_compl)
        val = (val == lbool::l_true) ? lbool::l_false : lbool::l_true;
      
      input_values.push_back(val);
    }

    return evaluate_lut(node_info.truth_table, input_values);
  }

  bool check_gate_consistency(lut_network::node n)
  {
    const auto& node_info = ntk_.get_node_info(n);
    const auto& fanins = node_info.fanins;

    std::vector<lbool> input_values;
    for (auto fanin_sig : fanins)
    {
      auto fanin_node = ntk_.get_node(fanin_sig);
      bool is_compl = ntk_.is_complemented(fanin_sig);
      lbool val = get_value(fanin_node);
      
      if (val == lbool::l_undef)
        return true;
      
      if (is_compl)
        val = (val == lbool::l_true) ? lbool::l_false : lbool::l_true;
      
      input_values.push_back(val);
    }

    lbool expected = evaluate_lut(node_info.truth_table, input_values);
    lbool actual = get_value(n);

    if (expected != actual && mode_ == solve_mode::DEBUG) {
      std::cout << "    CONFLICT at " << node_info.name 
                << ": Expected=" << (expected == lbool::l_true ? "TRUE" : "FALSE")
                << ", Actual=" << (actual == lbool::l_true ? "TRUE" : "FALSE") << std::endl;
    }

    return expected == actual;
  }

  lbool evaluate_lut(const std::string& tt_str, const std::vector<lbool>& inputs)
  {
    uint64_t tt = 0;
    if (tt_str.find("0x") == 0 || tt_str.find("0X") == 0)
      tt = std::stoull(tt_str, nullptr, 16);
    else
      tt = std::stoull(tt_str, nullptr, 10);

    uint64_t index = 0;
    for (size_t i = 0; i < inputs.size(); ++i)
    {
      if (inputs[i] == lbool::l_true)
        index |= (1ULL << i);
    }

    bool result = (tt >> index) & 1;
    return result ? lbool::l_true : lbool::l_false;
  }

  bool search()
  {
    if (!propagate())
      return false;

    lut_network::node unassigned = select_unassigned_variable();
    if (unassigned == static_cast<lut_network::node>(-1)) {
      if (mode_ == solve_mode::VERBOSE || mode_ == solve_mode::DEBUG) {
        std::cout << "  All variables assigned! Solution found." << std::endl;
      }
      return true;
    }

    num_decisions_++;
    const std::string node_name = ntk_.get_node_info(unassigned).name;
    
    if (mode_ == solve_mode::DEBUG) {
      std::cout << "  Decision #" << num_decisions_ << ": Trying " << node_name 
                << " = TRUE (depth " << trail_.size() << ")" << std::endl;
    }
    
    size_t saved_trail_size = trail_.size();
    
    set_value(unassigned, lbool::l_true, trail_.size());
    if (search())
      return true;

    if (mode_ == solve_mode::DEBUG) {
      std::cout << "  Backtracking from " << node_name << " = TRUE" << std::endl;
    }
    backtrack(saved_trail_size);

    num_decisions_++;
    if (mode_ == solve_mode::DEBUG) {
      std::cout << "  Decision #" << num_decisions_ << ": Trying " << node_name 
                << " = FALSE (depth " << trail_.size() << ")" << std::endl;
    }
    
    set_value(unassigned, lbool::l_false, trail_.size());
    if (search())
      return true;

    if (mode_ == solve_mode::DEBUG) {
      std::cout << "  Backtracking from " << node_name << " = FALSE" << std::endl;
    }
    backtrack(saved_trail_size);
    
    return false;
  }

  lut_network::node select_unassigned_variable()
  {
    for (auto pi : ntk_.get_pis())
    {
      if (get_value(pi) == lbool::l_undef)
        return pi;
    }

    for (size_t i = 1; i < ntk_.size(); ++i)
    {
      if (!ntk_.is_constant(i) && get_value(i) == lbool::l_undef)
        return i;
    }

    return static_cast<lut_network::node>(-1);
  }

  void backtrack(size_t target_size)
  {
    while (trail_.size() > target_size)
    {
      auto n = trail_.back();
      trail_.pop_back();
      values_[n] = lbool::l_undef;
      levels_[n] = -1;
    }
  }

private:
  const lut_network& ntk_;
  std::vector<lbool> values_;
  std::vector<int> levels_;
  std::vector<lut_network::node> trail_;
  uint64_t num_decisions_ = 0;
  uint64_t num_conflicts_ = 0;
  uint64_t num_implications_ = 0;
  solve_mode mode_;
};

} // namespace cirsat

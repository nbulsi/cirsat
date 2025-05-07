/**
 * @file aig_dpll_solver.hpp
 * @brief analog DPLL algorithm in CNF SAT solver
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_AIG_DPLL_SOLVER_HPP
#define CIRSAT_AIG_DPLL_SOLVER_HPP

#include "aig.hpp"
#include <vector>
#include <optional>

namespace cirsat {

// Class for solving AIG network
class aig_dpll_solver {
    private:
        // Reference to the AIG network
        const aig_ntk& m_ntk;
        
        // Node evaluation status and values
        std::vector<bool> m_values;
        std::vector<bool> m_assigned;
    
    public:
        explicit aig_dpll_solver(const aig_ntk& ntk) 
            : m_ntk(ntk), 
              m_values(ntk.get_gates().size(), false),
              m_assigned(ntk.get_gates().size(), false) {}
    
        // Clear all assignments
        void clear_assignments() {
            std::fill(m_assigned.begin(), m_assigned.end(), false);
        }
    
        // Assign value to input node
        void assign_input(GateId id, bool val) {
            m_values[id] = val;
            m_assigned[id] = true;
        }
    
        // Unassign input node
        void unassign_input(GateId id) {
            m_assigned[id] = false;
        }
    
        // Evaluate node value recursively
        bool evaluate_node(GateId id, uint32_t complement = 0) {
            if (m_assigned[id]) return m_values[id];
    
            const auto& node = m_ntk.get_gates()[id];
            
            // Input node without assignment
            if (node.children[0] == id && node.children[1] == id) {
                return m_values[id];
            }
    
            // Evaluate AND gate
            bool lhs_val = evaluate_node(m_ntk.data_to_index(node.children[0]));
            bool rhs_val = evaluate_node(m_ntk.data_to_index(node.children[1]));
            
            // Handle complemented edges
            if (m_ntk.data_to_complement(node.children[0])) lhs_val = !lhs_val;
            if (m_ntk.data_to_complement(node.children[1])) rhs_val = !rhs_val;
            // Check complement and assign value
            if (complement == 1) {
                m_values[id] = !(lhs_val & rhs_val); 
            } else {
                m_values[id] = lhs_val & rhs_val; 
            }
            m_assigned[id] = true;
            return m_values[id];
        }
    
        // Recursive search for satisfying assignment
        bool search(size_t input_idx, std::vector<bool>& input_vals) {
            const auto& inputs = m_ntk.get_inputs();
            if (input_idx == inputs.size()) {
                clear_assignments();
                for (size_t i = 0; i < inputs.size(); ++i) {
                    assign_input(inputs[i], input_vals[i]);
                }
                
                // Check all outputs
                for (GateId out_id : m_ntk.get_outputs()) {
                    bool out_val = evaluate_node(out_id >> 1, out_id & 1);
                    if (!out_val) return false;
                }
                return true;
            }
    
            // Try both possible input values
            for (bool val : {false, true}) {
                input_vals[input_idx] = val;
                if (search(input_idx + 1, input_vals)) return true;
            }
            return false;
        }
    
        // Main SAT solving function
        bool solve(std::vector<bool>& solution) {
            solution.resize(m_ntk.get_inputs().size());
            return search(0, solution);
        }
    };

    std::pair<bool, std::optional<std::vector<bool>>> solve_aig(const aig_ntk& ntk) {
        aig_dpll_solver solver(ntk);
        std::vector<bool> solution;
        
        bool is_sat = solver.solve(solution);
        
        if (is_sat) {
            return {true, std::move(solution)};
        } else {
            return {false, std::nullopt};
        }
    }

} // namespace cirsat

#endif // CIRSAT_AIG_DPLL_SOLVER_HPP

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
        std::vector<GateId> trail_node; 
        std::vector<size_t> node_level;

    
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
        
        void assign_node(GateId id, bool val) {
            if (m_assigned[id]) return; 
            m_values[id] = val;
            m_assigned[id] = true;
            trail_node.push_back(id);
        }
        // Unassign input node
        void unassign_input(GateId id) {
            m_assigned[id] = false;
        }
        // backtrack to the previous decision level
        void backtrack() {
            size_t node_start = node_level.back();
            node_level.pop_back();
            while (trail_node.size()>node_start) {
                GateId id = trail_node.back();
                trail_node.pop_back();
                m_assigned[id] = false;
                m_values[id] = false;
            }
        }
        
        //bcp
        void bcp(GateId id){
            const auto& n = m_ntk.get_gates()[id];
            for(auto out_id : n.outputs){
                const auto& out = m_ntk.get_gates()[out_id];
                if(m_assigned[out_id]) continue;
                GateId left_node = m_ntk.data_to_index(out.children[0]);
                GateId right_node = m_ntk.data_to_index(out.children[1]);
                if(m_assigned[left_node] && m_assigned[right_node]){
                    bool left_val , right_val;
                    if(m_ntk.data_to_complement(out.children[0])==1){
                        left_val = !m_values[left_node];
                    }
                    else{
                        left_val = m_values[left_node];
                    }
                    if(m_ntk.data_to_complement(out.children[1])==1){
                        right_val = !m_values[right_node];
                    }
                    else{
                        right_val = m_values[right_node];
                    }
                    bool out_val = left_val & right_val;
                    assign_node(out_id,out_val);
                    bcp(out_id);
                }
            }
        }
        
        // Recursive search for satisfying assignment
        bool search(size_t input_idx, std::vector<bool>& input_vals) {
            const auto& inputs = m_ntk.get_inputs();
            if (input_idx == inputs.size()) return false;
            // Try both possible input values
            for (bool val : {false, true}) {
                node_level.push_back(trail_node.size());
                GateId input_id = inputs[input_idx];
                input_vals[input_idx] = val;
                assign_node(input_id, val);
                bcp(input_id);
                auto out_data=m_ntk.get_outputs()[0];
                if(m_assigned[m_ntk.data_to_index(out_data)]){
                    if((m_ntk.data_to_complement(out_data)==0)&&m_values[m_ntk.data_to_index(out_data)]) {return true;}
                    else if((m_ntk.data_to_complement(out_data)==1)&&!m_values[m_ntk.data_to_index(out_data)]) {return true;}
                    else {
                        backtrack();
                        continue;
                    }
                }
                if (search(input_idx + 1, input_vals)) return true;

                backtrack();
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

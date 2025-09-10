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
#include <algorithm>


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
        std::vector<GateId> sorted_inputs;
    
    public:
        explicit aig_dpll_solver(const aig_ntk& ntk) 
            : m_ntk(ntk), 
              m_values(ntk.get_gates().size(), false),
              m_assigned(ntk.get_gates().size(), false) {}
    
        // Clear all assignments
        void clear_assignments() {
            std::fill(m_assigned.begin(), m_assigned.end(), false);
        }
    
        void sort_inputs() {
            sorted_inputs = m_ntk.get_inputs();
            std::sort(sorted_inputs.begin(), sorted_inputs.end(), [this](GateId a, GateId b) {
                gate g_a = m_ntk.get_gates()[a];
                gate g_b = m_ntk.get_gates()[b];
                return g_a.outputs.size() > g_b.outputs.size();
            });
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
        
        bool po_first(){
            for(auto out:m_ntk.get_outputs()){
                GateId out_id = m_ntk.data_to_index(out);
                bool out_comp =m_ntk.data_to_complement(out);
                bool out_val =!out_comp;
                assign_node(out_id,out_val);
                if(!forward_bcp(out_id)) return false;
            }
            return true;
        }
        bool forward_bcp(GateId id){
            const auto& n = m_ntk.get_gates()[id];
            if(m_values[id]){
                for (int i = 0; i < 2; ++i) {
                    auto child = n.children[i];
                    GateId child_id = m_ntk.data_to_index(child);
                    bool child_compl = m_ntk.data_to_complement(child);
                    bool child_val = !child_compl;
    
                    if (m_assigned[child_id]) {
                        if (m_values[child_id] != child_val) return false;
                    } else {
                        assign_node(child_id, child_val);
                        if (!forward_bcp(child_id)) return false;
                    }
                }    
            }
            return true;
        }
        //bcp
        void bcp(GateId id){
            const auto& n = m_ntk.get_gates()[id];
            for(auto out_id : n.outputs){
                const auto& out = m_ntk.get_gates()[out_id];
                if(m_assigned[out_id]) continue;
                GateId left_node = m_ntk.data_to_index(out.children[0]);
                GateId right_node = m_ntk.data_to_index(out.children[1]);
                bool left_comp = m_ntk.data_to_complement(out.children[0]);
                bool right_comp = m_ntk.data_to_complement(out.children[1]);
                bool left_val , right_val;
                if(m_assigned[left_node]){
                    left_val = left_comp ? !m_values[left_node] : m_values[left_node];
                }
                if(m_assigned[right_node]){
                    right_val = right_comp ? !m_values[right_node] : m_values[right_node];
                }
                if((m_assigned[left_node]&&!left_val) || (m_assigned[right_node]&&!right_val)){
                    assign_node(out_id, false);
                    //std::cout<<"assign false:"<<out_id<<std::endl;
                    bcp(out_id);
                }else if(m_assigned[left_node] && m_assigned[right_node]){
                    bool out_val = left_val & right_val;
                    assign_node(out_id,out_val);
                    bcp(out_id);
                }
            }
        }
        
        // Recursive search for satisfying assignment
        bool search(size_t input_idx, std::vector<bool>& input_vals) {
            const auto& inputs = sorted_inputs;
            if (input_idx == 0) {
                for (size_t i = 0; i < inputs.size(); ++i) {
                }
                std::cout << std::endl;
            }
            if (input_idx == inputs.size()){
                return false;} 
            // Try both possible input values
            for (bool val : {false, true}) {

                GateId input_id = inputs[input_idx];
                if (m_assigned[input_id]) {
                    if (m_values[input_id] == val) {
                        if (search(input_idx + 1, input_vals)) return true;
                    }
                    continue;
                }
                node_level.push_back(trail_node.size());
                input_vals[input_id-1] = val;
                assign_node(input_id, val);
                bcp(input_id);
                bool all_po_assigned = true;
                bool all_po_true = true;
        
                for (auto out_data : m_ntk.get_outputs()) {
                    GateId out_id = m_ntk.data_to_index(out_data);
                    bool out_compl = m_ntk.data_to_complement(out_data);
                    bool expected_val = !out_compl;
        
                    if (!m_assigned[out_id]) {
                        all_po_assigned = false;
                        break;
                    }
        
                    if (m_values[out_id] != expected_val) {
                        all_po_true = false;
                    }
                }
        
                if (all_po_assigned) {
                    if (all_po_true) {
                        return true;
                    } else {
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
            if(!po_first()){
                return false;
            }   
            clear_assignments(); 
            sort_inputs();
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

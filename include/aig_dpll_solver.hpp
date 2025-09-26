/**
 * @file aig_dpll_solver.hpp
 * @brief analog DPLL algorithm in CNF SAT solver
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_AIG_DPLL_SOLVER_HPP
#define CIRSAT_AIG_DPLL_SOLVER_HPP

#include "aig.hpp"
#include <optional>
<<<<<<< HEAD
=======
#include <algorithm>
>>>>>>> 9ae0ab3 (Implement watch-value table and implication tables)

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
<<<<<<< HEAD
        std::vector<size_t> node_level;

    
=======
        Direct_ImplicationTable di_table;
        Indirect_ImplicationTable ii_table;
        std::unordered_map<GateId, Watch_Values> watch_vals;
        std::vector<float> m_activity;

        struct DecInfor{
            size_t trail_start_index;
            uint32_t dec_line;
            std::unordered_set<GateId> j_nodes;
        };
        std::vector<DecInfor> mdi;
>>>>>>> 9ae0ab3 (Implement watch-value table and implication tables)
    public:
        explicit aig_dpll_solver(const aig_ntk& ntk) 
            : m_ntk(ntk), 
              m_values(ntk.get_gates().size(), false),
<<<<<<< HEAD
              m_assigned(ntk.get_gates().size(), false) {}
    
        // Clear all assignments
        void clear_assignments() {
            std::fill(m_assigned.begin(), m_assigned.end(), false);
        }
    
        // Assign value to input node
        void assign_input(GateId id, bool val) {
            m_values[id] = val;
            m_assigned[id] = true;
=======
              m_assigned(ntk.get_gates().size(), false)
        {

            ii_table[0].resize(m_ntk.get_gates().size());
            ii_table[1].resize(m_ntk.get_gates().size());
            m_ntk.build_implication_table(di_table, ii_table, watch_vals);

>>>>>>> 9ae0ab3 (Implement watch-value table and implication tables)
        }
        
        void assign_node(GateId id, bool val) {
            if (m_assigned[id]){
                std::cout << "[assign_node] Gate " << id << " already assigned, skip.\n";
                return; 
            } 
            
            if (id >= m_values.size()) {
                std::cerr << "[assign_node][ERROR] id " << id << " out of range! m_values.size=" << m_values.size() << std::endl;
                return;
            }

            m_values[id] = val;
            m_assigned[id] = true;
            trail_node.push_back(id);

            if(id>m_ntk.get_num_pis() && !val && all_inputs_unassigned(id)){
                mdi.back().j_nodes.insert(id);
            }


        }
        bool all_inputs_unassigned(const GateId id){
            const auto& g = m_ntk.get_gates()[id]; 
            for(const auto &child : g.children){
                GateId in_id =m_ntk.data_to_index(child);
                if(m_assigned[in_id]) return false;
            }
            return true;
        }

        void clean_jnodes(std::unordered_set<GateId>& jnodes){
            for(auto it = jnodes.begin(); it!= jnodes.end();){
                GateId id = *it;
                bool output_is_zero = m_assigned[id] && (m_values[id] == false);
                bool inputs_unassigned = all_inputs_unassigned(id);
                if(!(output_is_zero && inputs_unassigned ) ){    
                    it = jnodes.erase(it);
                }else ++it;
            }
        }
<<<<<<< HEAD
        
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
=======
       
        bool po_first(){
            mdi.clear();
            mdi.emplace_back();
            mdi.back().trail_start_index = trail_node.size();

            for(auto out:m_ntk.get_outputs()){
                GateId out_id = m_ntk.data_to_index(out);
                bool out_comp =m_ntk.data_to_complement(out);
                bool out_val =!out_comp;

                assign_node(out_id,out_val);
                mdi[0].dec_line=out_id;

                if(!BCP(out_id)){
                    return false;
                } 
            }
            return true;
        }

        GateId pick_from_j_node(){
            if(mdi.empty()) return INVALID_GATE;
            auto &jnodes = mdi.back().j_nodes;
            if(jnodes.empty()) return INVALID_GATE;
            return *jnodes.begin();
        }
        
        
        bool BCP(GateId id){
            bool val = m_values[id];
            auto it = di_table.find(id);
            if (it != di_table.end()){
                for (const auto& [next,next_val] : it ->second[val]){
                    if (next >= m_values.size()) {
                    }

                    if (m_assigned[next]){
                        if (m_values[next] != next_val) {
                            return false; // Conflict
                        }
                    } else {
                        assign_node(next, next_val);
                        if(!BCP(next)){
                            return false;
                        }
                    }
                }
            }

            for (const auto& out : ii_table[val][id]){
                const auto& gate = m_ntk.get_gates()[out];
                GateId left_node = m_ntk.data_to_index(gate.children[0]);
                GateId right_node = m_ntk.data_to_index(gate.children[1]);

                bool left_wv = watch_vals[out].input1;
                bool right_wv = watch_vals[out].input2;
                bool out_wv = watch_vals[out].output;

                bool left_match = false;
                bool right_match = false;
                bool out_match = false;

                if(m_assigned[left_node] && (m_values[left_node] == watch_vals[out].input1)){left_match = true;}
                if(m_assigned[right_node] && (m_values[right_node] == watch_vals[out].input2)){right_match = true;}
                if(m_assigned[out] && (m_values[out] == watch_vals[out].output)){out_match = true;}
                int wv_count = left_match + right_match + out_match;

                int assigned_count = m_assigned[left_node] + m_assigned[right_node] + m_assigned[out];

                if(assigned_count == 3) {
                    if((wv_count == 3)||(wv_count == 0)) {
                        return false;
                    }
                    if(wv_count == 2) {
                        continue;
                    }
                    if(wv_count ==1) {
                        if((left_match)||(right_match)){
                            return false;

                        }else{
                            continue;
                        }
                    }
                }

                if (wv_count<assigned_count){
                    continue;
                }

                if(assigned_count == 1) {
                    continue;
                }


                if(assigned_count == 2) {
                    if (!m_assigned[left_node]) {
                        assign_node(left_node, !left_wv);
                        if(!BCP(left_node)){
                            return false;
                        }
                    }
                    if (!m_assigned[right_node]) {
                        assign_node(right_node, !right_wv);
                        if(!BCP(right_node)){
                            return false;
                        }
                    }
                    if (!m_assigned[out]) {
                        assign_node(out, true);
                        if(!BCP(out)){
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        bool search(std::vector<bool>& input_vals) {

            if (mdi.empty()) {
                mdi.emplace_back();
                mdi.back().trail_start_index = trail_node.size();
            }
            clean_jnodes(mdi.back().j_nodes);
            
            if(mdi.back().j_nodes.empty()){
                for(GateId pi : m_ntk.get_inputs()){
                    if(m_assigned[pi]){
                        input_vals[pi-1]=m_values[pi];
                    }
                    else{
                        input_vals[pi-1]=0;
                    }
                }
                return true;
            }

            GateId jnode = pick_from_j_node();
            const auto& g = m_ntk.get_gates()[jnode]; 
            GateId gate = m_ntk.data_to_index(g.children[0]);
            
            if (gate == INVALID_GATE){

                std::cout << "[search] gate wrong!!!\n";

            }

            DecInfor newframe;
            newframe.dec_line = gate;
            newframe.trail_start_index = trail_node.size();
            newframe.j_nodes = mdi.back().j_nodes;
            auto saved_jnodes = newframe.j_nodes;
            mdi.push_back(std::move(newframe));


            for (bool val : {false, true}) {
                if (m_assigned[gate]) {

                    if (m_values[gate] != val) {
                        continue;
                    } else {
                        if (search(input_vals)) return true;
                        continue;
                    }
                }
>>>>>>> 9ae0ab3 (Implement watch-value table and implication tables)

                assign_node(gate, val);
                
                if (!BCP(gate)) {

                    while (trail_node.size() > mdi.back().trail_start_index) {
                        GateId id = trail_node.back();
                        trail_node.pop_back();
                        m_assigned[id] = false;
                        m_values[id] = false;
                    }
                    mdi.back().j_nodes=saved_jnodes;
                    continue;
                }

                if (search(input_vals)) return true;
                while (trail_node.size() > mdi.back().trail_start_index) {
                    GateId id = trail_node.back();
                    trail_node.pop_back();
                    m_assigned[id] = false;
                    m_values[id] = false;
                }
                mdi.back().j_nodes=saved_jnodes;
            }
            mdi.pop_back();
            if (mdi.size() == 1) {
                return false;
            }
            return false;
        }
        

        // Main SAT solving function
        bool solve(std::vector<bool>& solution) {
            solution.resize(m_ntk.get_inputs().size());
<<<<<<< HEAD
            return search(0, solution);
=======
            if(!po_first()){
                return false;
            }    
            return search(solution);
>>>>>>> 9ae0ab3 (Implement watch-value table and implication tables)
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

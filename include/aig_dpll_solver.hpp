/**
 * @file aig_dpll_solver.hpp
 * @brief analog DPLL algorithm in CNF SAT solver
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_AIG_DPLL_SOLVER_HPP
#define CIRSAT_AIG_DPLL_SOLVER_HPP

#include "aig.hpp"
#include <optional>
#include <algorithm>

namespace cirsat {

class aig_dpll_solver {
  private:
        aig_ntk m_ntk;

        std::vector<bool> m_values;
        std::vector<bool> m_assigned;
         std::vector<GateId> trail_node;
        Direct_ImplicationTable di_table;
        Indirect_ImplicationTable ii_table;
        std::unordered_map<GateId, Watch_Values> watch_vals;
        std::unordered_map<GateId, std::vector<bool>> learn_watch_points;
        std::unordered_map<GateId, std::vector<bool>> learn_watch_vals;
        std::vector<float> m_activity;
        float m_activity_inc = 1.0f;
        float m_activity_decay = 0.95f;
        float m_activity_rescale = 1e-20f;
        std::vector<std::vector<GateId>> source;
        std::vector<GateId> conf_lines;
        std::vector<int> decision_level;
        int cur_level = 0;
        uint32_t m_original_gate_size = 0;
        std::vector<uint32_t> m_inq_stamp;
        uint32_t m_inq_cur_stamp = 1;

        struct DecInfor{
            size_t trail_start_index;
            std::unordered_set<GateId> j_nodes;
        };
        std::vector<DecInfor> mdi;
        uint64_t m_conflicts = 0;
        uint64_t m_restart_limit = 500;
        double   m_restart_mult = 1.2;
        uint64_t m_restart_max_limit = 20000;

  public:

        explicit aig_dpll_solver(const aig_ntk& ntk)
            : m_ntk(ntk),
              m_values(ntk.get_gates().size(), false),
              m_assigned(ntk.get_gates().size(), false)
        {

            m_ntk.build_implication_table(di_table, ii_table, watch_vals);

            source.resize(m_ntk.get_gates().size());
            decision_level.resize(m_ntk.get_gates().size(), -1);
            for (auto &v : source) v.reserve(2);
            trail_node.reserve(m_ntk.get_gates().size());
            m_inq_stamp.resize(m_ntk.get_gates().size(), 0);
        }

        void assign_node(GateId id, bool val) {
            assert(id < m_values.size() && !m_assigned[id]);
            m_values[id] = val;
            m_assigned[id] = true;
            decision_level[id] = cur_level;
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
        void rebuild_jnodes_level0_from_trail() {
            auto& jnodes = mdi.front().j_nodes;
            jnodes.clear();
            for (GateId id : trail_node) {
                if (id > m_ntk.get_num_pis() && !m_values[id] && all_inputs_unassigned(id)) {
                    jnodes.insert(id);
                }
            }
        }

        bool po_first(){
            mdi.clear();
            mdi.emplace_back();
            mdi.back().trail_start_index = trail_node.size();

            if (m_original_gate_size != 0) {
                assign_node(0, false);
                if (!BCP(0)) return false;
            }
            for(auto out:m_ntk.get_outputs()){
                GateId out_id = m_ntk.data_to_index(out);
                bool out_comp =m_ntk.data_to_complement(out);
                bool out_val =!out_comp;
                cur_level = 0;
                if (m_assigned[out_id] && m_values[out_id] != out_val) {
                    return false;
                }
                if (m_assigned[out_id]) continue;
                assign_node(out_id, out_val);
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

            GateId best_gate = INVALID_GATE;
            float max_score = -1.0f;
            for (GateId j : jnodes) {

                const auto &gate = m_ntk.get_gates()[j];
                for (size_t i = 0; i < gate.children.size(); i++) {
                    auto child = gate.children[i];
                    GateId in_id = m_ntk.data_to_index(child);

                    float score =  m_activity[in_id];

                    if (score > max_score) {
                        max_score = score;
                        best_gate = in_id;
                    }

                }
            }
            return best_gate;
        }
        void bump_activity(GateId id) {
            m_activity[id] += m_activity_inc;
            if (m_activity[id] > 1e20f) {
                for (auto &a : m_activity) a *= m_activity_rescale;
                m_activity_inc *= m_activity_rescale;
            }
        }
        void initialize_activity()
        {
            const auto &gates = m_ntk.get_gates();
            m_activity.resize(gates.size(), 0.0f);

            for (GateId id = 1; id < gates.size(); ++id) {
                m_activity[id] = static_cast<float>(gates[id].outputs.size());

                if (id > m_ntk.get_num_pis()) {
                    m_activity[id] += 2.0f;
                }

            }
        }

        bool propagate(std::vector<GateId>& queue) {
            ++m_inq_cur_stamp;
            if (m_inq_cur_stamp == 0) {
                std::fill(m_inq_stamp.begin(), m_inq_stamp.end(), 0);
                m_inq_cur_stamp = 1;
            }
            auto enqueue = [&](GateId x) {
                if (m_inq_stamp[x] != m_inq_cur_stamp) {
                    m_inq_stamp[x] = m_inq_cur_stamp;
                    queue.push_back(x);
                }
            };

            while (!queue.empty()) {
                GateId id = queue.back();
                queue.pop_back();

                bool val = m_values[id];

                if (auto it = di_table.find(id); it != di_table.end()) {
                    for (const auto& [next, next_val] : it->second[val]) {
                        if (m_assigned[next]) {
                            if (m_values[next] != next_val) {
                                conf_lines.clear();

                                conf_lines = {id, next};
                                return false;
                            }
                            continue;
                        }
                        auto &src = source[next];
                        src.clear();
                        src.push_back(id);
                        assign_node(next, next_val);
                        enqueue(next);
                    }
                }

                auto &wlist = ii_table[val][id];
                const size_t end = wlist.size();
                size_t j = 0;

                auto preserve_remaining = [&](size_t first) {
                    for (size_t i = first; i < wlist.size(); ++i) wlist[j++] = wlist[i];
                    wlist.resize(j);
                };
                for (size_t i = 0; i < end; ++i) {
                    GateId out = wlist[i];

                    if (out < m_original_gate_size) {
                        const auto& g = m_ntk.get_gates()[out];
                        GateId left_node  = m_ntk.data_to_index(g.children[0]);
                        GateId right_node = m_ntk.data_to_index(g.children[1]);

                        bool left_wv  = watch_vals[out].input1;
                        bool right_wv = watch_vals[out].input2;
                        bool out_wv   = watch_vals[out].output;

                        bool left_match  = m_assigned[left_node]  && (m_values[left_node]  == left_wv);
                        bool right_match = m_assigned[right_node] && (m_values[right_node] == right_wv);
                        bool out_match   = m_assigned[out]        && (m_values[out]        == out_wv);

                        int wv_count = int(left_match) + int(right_match) + int(out_match);
                        int assigned_count = int(m_assigned[left_node]) + int(m_assigned[right_node]) + int(m_assigned[out]);

                        if (assigned_count == 3) {
                            if ((wv_count == 3) || (wv_count == 0) || (wv_count == 1 && (left_match || right_match))) {
                                conf_lines.clear();
                                conf_lines.push_back(left_node);
                                conf_lines.push_back(right_node);
                                conf_lines.push_back(out);
                                preserve_remaining(i);
                                return false;
                            }
                            wlist[j++] = out;
                            continue;
                        }

                        if (wv_count < assigned_count) {
                            wlist[j++] = out;
                            continue;
                        }

                        if (assigned_count == 1) {
                            wlist[j++] = out;
                            continue;
                        }

                        if (assigned_count == 2) {
                            if (!m_assigned[left_node]) {
                                auto &src = source[left_node];
                                src.clear();
                                src.push_back(right_node);
                                src.push_back(out);
                                assign_node(left_node, !left_wv);
                                enqueue(left_node);
                            }
                            if (!m_assigned[right_node]) {
                                auto &src = source[right_node];
                                src.clear();
                                src.push_back(left_node);
                                src.push_back(out);
                                assign_node(right_node, !right_wv);
                                enqueue(right_node);
                            }
                            if (!m_assigned[out]) {
                                auto &src = source[out];
                                src.clear();
                                src.push_back(left_node);
                                src.push_back(right_node);
                                assign_node(out, true);
                                enqueue(out);
                            }
                            wlist[j++] = out;
                            continue;
                        }

                        wlist[j++] = out;
                        continue;
                    }

                    else {

                        auto lv_it = learn_watch_vals.find(out);
                        auto lp_it = learn_watch_points.find(out);
                        if (lv_it == learn_watch_vals.end() || lp_it == learn_watch_points.end()) {
                            continue;
                        }

                        const auto &lwv = lv_it->second;
                        auto &lpoints = lp_it->second;

                        const auto &lg = m_ntk.get_gates()[out];
                        const auto &fanins = lg.fanins;

                        size_t idx_trig = SIZE_MAX;
                        for (size_t t = 0; t < fanins.size(); ++t) {
                            if (fanins[t] == id) { idx_trig = t; break; }
                        }
                        if (idx_trig == SIZE_MAX) {
                            continue;
                        }
                        if (!lpoints[idx_trig] || lwv[idx_trig] != val) {
                            continue;
                        }

                        size_t k = fanins.size();
                        size_t count_watch = 0;
                        size_t count_unassigned = 0;
                        GateId last_unassigned = INVALID_GATE;

                        bool satisfied = false;
                        for (size_t t = 0; t < k; ++t) {
                            GateId fin = fanins[t];
                            if (!m_assigned[fin]) {
                                ++count_unassigned;
                                last_unassigned = fin;
                            } else {
                                if (m_values[fin] == lwv[t]) {
                                    ++count_watch;
                                } else {
                                    satisfied = true;
                                    break;
                                }
                            }
                        }

                        if (satisfied) {
                            wlist[j++] = out;
                            continue;
                        }

                        if (count_unassigned >= 2) {
                            size_t new_idx = SIZE_MAX;
                            for (size_t t = 0; t < k; ++t) {
                                if (t == idx_trig) continue;
                                if (lpoints[t]) continue;
                                GateId fin = fanins[t];
                                if (!m_assigned[fin]) { new_idx = t; break; }
                            }

                            if (new_idx != SIZE_MAX) {
                                lpoints[idx_trig] = false;
                                lpoints[new_idx] = true;

                                GateId new_fin = fanins[new_idx];
                                bool new_wv = lwv[new_idx];
                                ii_table[new_wv][new_fin].push_back(out);

                                continue;
                            }
                        }

                        if (count_watch == k) {
                            conf_lines.clear();
                            conf_lines = fanins;
                            preserve_remaining(i);
                            return false;
                        }

                        if (count_unassigned == 1 && count_watch == k - 1 && last_unassigned != INVALID_GATE) {
                            size_t idx_u = SIZE_MAX;
                            for (size_t t = 0; t < k; ++t) {
                                if (fanins[t] == last_unassigned) { idx_u = t; break; }
                            }
                            if (idx_u != SIZE_MAX) {
                                bool assign_val = !lwv[idx_u];

                                auto &src = source[last_unassigned];
                                src.clear();
                                for (size_t t = 0; t < k; ++t) {
                                    if (t == idx_u) continue;
                                    src.push_back(fanins[t]);
                                }

                                assign_node(last_unassigned, assign_val);
                                enqueue(last_unassigned);
                            }
                        }

                        wlist[j++] = out;
                        continue;
                    }
                }

                wlist.resize(j);

            }
            return true;
        }

        bool BCP(GateId start) {

            std::vector<GateId> queue;
            queue.reserve(64);
            queue.push_back(start);
            return propagate(queue);
        }

        void backtrack(int target_level){
            if (target_level >= cur_level) return;
            size_t keep_trail_index;
            if (target_level + 1 < (int)mdi.size()) {
                keep_trail_index = mdi[target_level + 1].trail_start_index;
            } else {
                keep_trail_index = mdi[target_level].trail_start_index;
            }
            for (int i = (int)trail_node.size() - 1; i >= (int)keep_trail_index; --i)
            {
                GateId n = trail_node[i];
                m_assigned[n] = false;
                decision_level[n] = -1;
                source[n].clear();
            }
            trail_node.resize(keep_trail_index);
            mdi.resize(target_level + 1);
            cur_level = target_level;
        }

        void restart_to_level0() {

            backtrack(0);
            conf_lines.clear();
            rebuild_jnodes_level0_from_trail();

            cur_level = 0;
        }

        bool conflict() {
            while (!conf_lines.empty()) {
                std::unordered_set<GateId> clause;
                // Resolve backward through the trail until one literal remains at the conflict level.
                int conflict_level = 0;
                for (GateId id : conf_lines) {
                    if (decision_level[id] > 0) {
                        clause.insert(id);
                        conflict_level = std::max(conflict_level, decision_level[id]);
                    }
                }
                if (clause.empty()) return false;

                size_t at_level = 0;
                for (GateId id : clause) {
                    if (decision_level[id] == conflict_level) ++at_level;
                }
                GateId uip = INVALID_GATE;
                for (auto it = trail_node.rbegin(); it != trail_node.rend(); ++it) {
                    const auto id = *it;
                    if (decision_level[id] != conflict_level || !clause.count(id)) continue;
                    if (at_level == 1) {
                        uip = id;
                        break;
                    }
                    assert(!source[id].empty());
                    clause.erase(id);
                    --at_level;
                    for (GateId reason : source[id]) {
                        if (decision_level[reason] > 0 && clause.insert(reason).second &&
                            decision_level[reason] == conflict_level) {
                            ++at_level;
                        }
                    }
                }
                assert(uip != INVALID_GATE);

                const bool value = !m_values[uip];
                int target_level = 0;
                std::vector<GateId> learned{uip};
                std::vector<bool> forbidden{!value};
                std::vector<GateId> reasons;
                for (GateId id : clause) {
                    bump_activity(id);
                    if (id == uip) continue;
                    target_level = std::max(target_level, decision_level[id]);
                    learned.push_back(id);
                    forbidden.push_back(m_values[id]);
                    reasons.push_back(id);
                }
                m_activity_inc /= m_activity_decay;

                if (!reasons.empty()) {
                    // Watch the asserting literal and a literal at the backjump level.
                    const auto second = std::max_element(learned.begin() + 1, learned.end(),
                        [&](GateId lhs, GateId rhs) { return decision_level[lhs] < decision_level[rhs]; });
                    const auto index = second - learned.begin();
                    std::iter_swap(learned.begin() + 1, second);
                    const bool second_value = forbidden[index];
                    forbidden[index] = forbidden[1];
                    forbidden[1] = second_value;
                    m_ntk.create_or_learning_gate(
                        learned, forbidden, ii_table, learn_watch_vals, learn_watch_points);
                    const auto size = m_ntk.get_gates().size();
                    m_values.resize(size, false);
                    m_assigned.resize(size, false);
                    decision_level.resize(size, -1);
                    source.resize(size);
                   m_inq_stamp.resize(size, 0);
                    m_activity.resize(size, 0.0f);
                }

                backtrack(target_level);
                source[uip] = std::move(reasons);
                assign_node(uip, value);
                conf_lines.clear();
                if (BCP(uip)) return true;
            }
            return false;
        }

        bool cdcl_search(std::vector<bool>& input_vals){
            if (mdi.empty()) { mdi.emplace_back(); mdi.back().trail_start_index = trail_node.size(); }
            conf_lines.clear();

            while(true){
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

                GateId gate = pick_from_j_node();
                assert(gate != INVALID_GATE);

                cur_level++;
                DecInfor newframe;
                newframe.trail_start_index = trail_node.size();
                newframe.j_nodes = mdi.back().j_nodes;
                mdi.push_back(std::move(newframe));

                assign_node(gate,false);

                if(!BCP(gate)){

                    ++m_conflicts;

                    if(!conflict()){
                        return false;
                    }

                    if (m_conflicts >= m_restart_limit) {
                        m_conflicts = 0;
                        restart_to_level0();

                        uint64_t new_limit = static_cast<uint64_t>(m_restart_limit * m_restart_mult);
                        if (new_limit <= m_restart_limit) new_limit = m_restart_limit + 1;
                        if (new_limit > m_restart_max_limit) new_limit = m_restart_max_limit;
                        m_restart_limit = new_limit;

                    }
                }
            }

        }

        bool solve(std::vector<bool>& solution) {
            m_original_gate_size = m_ntk.get_gates().size();
            solution.resize(m_ntk.get_inputs().size());
            initialize_activity();
            return po_first() && cdcl_search(solution);
        }
    };

    inline std::pair<bool, std::optional<std::vector<bool>>> solve_aig(const aig_ntk& ntk) {
        aig_dpll_solver solver(ntk);
        std::vector<bool> solution;

        bool is_sat = solver.solve(solution);

        if (is_sat) {
            return {true, std::move(solution)};
        } else {
            return {false, std::nullopt};
        }
    }

}

#endif

/**
 * @file aig_dpll_solver.hpp
 * @brief analog DPLL algorithm in CNF SAT solver
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_AIG_DPLL_SOLVER_HPP
#define CIRSAT_AIG_DPLL_SOLVER_HPP

#include "aig.hpp"
#include <algorithm>
#include <ctime>
#include <optional>
namespace cirsat {

// Class for solving AIG network
class aig_dpll_solver
{
  private:
    // Reference to the AIG network
    aig_ntk& m_ntk;

    // Node evaluation status and values
    std::vector<bool> m_values;
    std::vector<bool> m_assigned;
    std::vector<GateId> trail_node;
    Direct_ImplicationTable di_table;
    Indirect_ImplicationTable ii_table;
    std::unordered_map<GateId, Watch_Values> watch_vals;
    std::unordered_map<GateId, std::vector<bool>> learn_watch_vals;
    std::vector<float> m_activity;
    std::vector<std::vector<GateId>> source;
    std::vector<GateId> conf_lines;
    std::vector<int> decision_level;
    int cur_level = 0;
    uint32_t m_original_gate_size = 0;
    uint64_t learning_gate_count = 0;

    struct DecInfor {
        size_t trail_start_index;
        uint32_t dec_line;
        std::unordered_set<GateId> j_nodes;
    };
    std::vector<DecInfor> mdi;

  public:
    explicit aig_dpll_solver( aig_ntk& ntk )
        : m_ntk( ntk ), m_values( ntk.get_gates().size(), false ), m_assigned( ntk.get_gates().size(), false )
    {

        ii_table[0].resize( m_ntk.get_gates().size() );
        ii_table[1].resize( m_ntk.get_gates().size() );
        m_ntk.build_implication_table( di_table, ii_table, watch_vals );
        source.resize( m_ntk.get_gates().size() );
        decision_level.resize( m_ntk.get_gates().size(), -1 );
    }

    void assign_node( GateId id, bool val, const std::vector<GateId>& from = {} )
    {
        if ( m_assigned[id] )
        {
            return;
        }

        if ( id >= m_values.size() )
        {
        }

        m_values[id] = val;
        m_assigned[id] = true;
        source[id] = from;
        decision_level[id] = cur_level;
        trail_node.push_back( id );

        if ( id > m_ntk.get_num_pis() && !val && all_inputs_unassigned( id ) )
        {

            mdi.back().j_nodes.insert( id );
        }
    }
    bool all_inputs_unassigned( const GateId id )
    {
        const auto& g = m_ntk.get_gates()[id];
        for ( const auto& child : g.children )
        {
            GateId in_id = m_ntk.data_to_index( child );
            if ( m_assigned[in_id] )
                return false;
        }
        return true;
    }
    void clean_jnodes( std::unordered_set<GateId>& jnodes )
    {
        for ( auto it = jnodes.begin(); it != jnodes.end(); )
        {
            GateId id = *it;
            bool output_is_zero = m_assigned[id] && ( m_values[id] == false );
            bool inputs_unassigned = all_inputs_unassigned( id );
            if ( !( output_is_zero && inputs_unassigned ) )
            {
                it = jnodes.erase( it );
            }
            else
                ++it;
        }
    }

    bool po_first()
    {
        mdi.clear();
        mdi.emplace_back();
        mdi.back().trail_start_index = trail_node.size();

        for ( auto out : m_ntk.get_outputs() )
        {
            GateId out_id = m_ntk.data_to_index( out );
            bool out_comp = m_ntk.data_to_complement( out );
            bool out_val = !out_comp;
            cur_level = 0;
            assign_node( out_id, out_val, {} );
            mdi[0].dec_line = out_id;
            if ( !BCP( out_id ) )
            {
                return false;
            }
        }

        return true;
    }

    GateId pick_unassigned_gate()
    {
        GateId best = INVALID_GATE;
        float max_activity = -1.0f;
        for ( GateId id = 1; id < m_activity.size(); ++id )
        {
            if ( !m_assigned[id] && m_activity[id] > max_activity )
            {
                max_activity = m_activity[id];
                best = id;
            }
        }
        return best;
    }
    GateId pick_from_j_node()
    {
        if ( mdi.empty() )
            return INVALID_GATE;
        auto& jnodes = mdi.back().j_nodes;
        if ( jnodes.empty() )
            return INVALID_GATE;

        GateId best_gate = INVALID_GATE;
        size_t max_fanout = 0;
        for ( GateId j : jnodes )
        {
            const auto& gate = m_ntk.get_gates()[j];
            for ( auto child : gate.children )
            {
                GateId in_id = m_ntk.data_to_index( child );
                if ( m_assigned[in_id] )
                    continue;
                size_t fanout = m_ntk.get_gates()[in_id].outputs.size();

                if ( fanout > max_fanout )
                {
                    max_fanout = fanout;
                    best_gate = in_id;
                }
            }
        }
        return best_gate;
    }

    bool BCP( GateId id )
    {

        bool val = m_values[id];
        auto it = di_table.find( id );
        if ( it != di_table.end() )
        {
            for ( const auto& [next, next_val] : it->second[val] )
            {
                if ( next >= m_values.size() )
                {
                }

                if ( m_assigned[next] )
                {
                    if ( m_values[next] != next_val )
                    {
                        conf_lines.clear();
                        std::unordered_set<GateId> in_conf;
                        for ( auto g : source[id] )
                        {
                            if ( in_conf.insert( g ).second )
                            {
                                conf_lines.push_back( g );
                            }
                        }

                        if ( in_conf.insert( next ).second )
                        {
                            conf_lines.push_back( next );
                        }
                        return false;
                    }
                }
                else
                {
                    std::vector<GateId> from = source[id];
                    from.push_back( id );
                    assign_node( next, next_val, from );
                    if ( !BCP( next ) )
                    {
                        return false;
                    }
                }
            }
        }

        for ( const auto& out : ii_table[val][id] )
        {
            if ( out < m_original_gate_size )
            {
                const auto& gate = m_ntk.get_gates()[out];
                GateId left_node = m_ntk.data_to_index( gate.children[0] );
                GateId right_node = m_ntk.data_to_index( gate.children[1] );

                bool left_wv = watch_vals[out].input1;
                bool right_wv = watch_vals[out].input2;
                bool out_wv = watch_vals[out].output;

                bool left_match = false;
                bool right_match = false;
                bool out_match = false;

                if ( m_assigned[left_node] && ( m_values[left_node] == watch_vals[out].input1 ) )
                {
                    left_match = true;
                }
                if ( m_assigned[right_node] && ( m_values[right_node] == watch_vals[out].input2 ) )
                {
                    right_match = true;
                }
                if ( m_assigned[out] && ( m_values[out] == watch_vals[out].output ) )
                {
                    out_match = true;
                }
                int wv_count = left_match + right_match + out_match;

                int assigned_count = m_assigned[left_node] + m_assigned[right_node] + m_assigned[out];

                if ( assigned_count == 3 )
                {
                    if ( ( wv_count == 3 ) || ( wv_count == 0 ) )
                    {
                        conf_lines.clear();
                        std::unordered_set<GateId> in_conf;
                        for ( GateId g : { left_node, right_node, out } )
                        {
                            if ( in_conf.insert( g ).second )
                            {
                                conf_lines.push_back( g );
                            }
                        }
                        return false;
                    }
                    if ( wv_count == 2 )
                    {
                        continue;
                    }
                    if ( wv_count == 1 )
                    {
                        if ( ( left_match ) || ( right_match ) )
                        {
                            conf_lines.clear();
                            std::unordered_set<GateId> in_conf;
                            for ( GateId g : { left_node, right_node, out } )
                            {
                                if ( in_conf.insert( g ).second )
                                {
                                    conf_lines.push_back( g );
                                }
                            }
                            return false;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }

                if ( wv_count < assigned_count )
                {
                    continue;
                }

                if ( assigned_count == 1 )
                {
                    continue;
                }

                if ( assigned_count == 2 )
                {
                    if ( !m_assigned[left_node] )
                    {
                        std::vector<GateId> from;
                        from.push_back( right_node );
                        from.push_back( out );
                        assign_node( left_node, !left_wv, from );
                        if ( !BCP( left_node ) )
                        {
                            return false;
                        }
                    }
                    if ( !m_assigned[right_node] )
                    {
                        std::vector<GateId> from;
                        from.push_back( left_node );
                        from.push_back( out );
                        assign_node( right_node, !right_wv, from );
                        if ( !BCP( right_node ) )
                        {
                            return false;
                        }
                    }
                    if ( !m_assigned[out] )
                    {
                        std::vector<GateId> from;
                        from.push_back( left_node );
                        from.push_back( right_node );
                        assign_node( out, true, from );
                        if ( !BCP( out ) )
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                const auto& lg = m_ntk.get_gates()[out];
                const auto& fanins = lg.fanins;
                const auto& lwv_it = learn_watch_vals.find( out );
                if ( lwv_it == learn_watch_vals.end() )
                    continue;

                const std::vector<bool>& lwv = lwv_it->second;
                size_t k = fanins.size();
                size_t count_watch = 0;
                size_t count_unassigned = 0;
                GateId last_unassigned = INVALID_GATE;

                for ( size_t i = 0; i < k; ++i )
                {
                    GateId fin = fanins[i];
                    if ( !m_assigned[fin] )
                    {
                        ++count_unassigned;
                        last_unassigned = fin;
                    }
                    else
                    {
                        bool fin_val = m_values[fin];
                        if ( fin_val == lwv[i] )
                            ++count_watch;
                    }
                }

                if ( count_watch == k )
                {
                    conf_lines.clear();
                    std::unordered_set<GateId> in_conf;
                    for ( GateId f : fanins )
                    {
                        if ( in_conf.insert( f ).second )
                            conf_lines.push_back( f );
                    }
                    return false;
                }

                // If exactly one unassigned and all others are watch => unit propagation
                if ( count_unassigned == 1 && count_watch == k - 1 && last_unassigned != INVALID_GATE )
                {
                    // find index of last_unassigned to get its wanted value (!watched)
                    size_t idx = SIZE_MAX;
                    for ( size_t i = 0; i < k; ++i )
                    {
                        if ( fanins[i] == last_unassigned )
                        {
                            idx = i;
                            break;
                        }
                    }
                    if ( idx == SIZE_MAX )
                        continue; // shouldn't happen

                    bool assign_val = !lwv[idx]; // non-watch value
                    // build from vector: include other fanins and the learn_gate
                    std::vector<GateId> from;
                    for ( size_t i = 0; i < k; ++i )
                    {
                        if ( i == idx )
                            continue;
                        from.push_back( fanins[i] );
                    }
                    assign_node( last_unassigned, assign_val, from );
                    if ( !BCP( last_unassigned ) )
                        return false;
                }
            }
        }
        return true;
    }

    void backtrack( int target_level )
    {
        if ( target_level >= cur_level )
            return;
        size_t keep_trail_index;
        if ( target_level + 1 < (int)mdi.size() )
        {
            keep_trail_index = mdi[target_level + 1].trail_start_index;
        }
        else
        {
            keep_trail_index = mdi[target_level].trail_start_index;
        }
        for ( int i = (int)trail_node.size() - 1; i >= (int)keep_trail_index; --i )
        {
            GateId n = trail_node[i];
            m_assigned[n] = false;
            decision_level[n] = -1;
            source[n].clear();
        }
        trail_node.resize( keep_trail_index );
        mdi.resize( target_level + 1 );
        cur_level = target_level;
    }

    bool conflict()
    {

        if ( conf_lines.empty() )
            return false;

        int num_at_cur_level = 0;
        std::unordered_set<GateId> in_conf( conf_lines.begin(), conf_lines.end() );
        int iteration = 0;
        do
        {
            iteration++;
            conf_lines.erase( std::remove_if( conf_lines.begin(), conf_lines.end(),
                                              [&]( GateId x ) { return decision_level[x] == 0; } ),
                              conf_lines.end() );
            if ( conf_lines.empty() )
            {
                return false;
            }

            num_at_cur_level = 0;
            std::vector<GateId> expand_list;
            for ( GateId g : conf_lines )
            {
                if ( decision_level[g] == cur_level )
                {
                    num_at_cur_level++;
                    if ( !source[g].empty() )
                    {
                        expand_list.push_back( g );
                    }
                }
            }
            if ( num_at_cur_level == 1 )
            {
                break;
            }
            if ( expand_list.empty() )
            {
                break;
            }
            for ( GateId g : expand_list )
            {
                in_conf.erase( g );
                for ( GateId src : source[g] )
                {
                    if ( in_conf.insert( src ).second )
                    {
                        conf_lines.push_back( src );
                    }
                }
            }
            conf_lines.erase( std::remove_if( conf_lines.begin(), conf_lines.end(),
                                              [&]( GateId x ) {
                                                  return std::find( expand_list.begin(), expand_list.end(), x ) !=
                                                         expand_list.end();
                                              } ),
                              conf_lines.end() );

        } while ( true );

        conf_lines.erase(
            std::remove_if( conf_lines.begin(), conf_lines.end(), [&]( GateId x ) { return decision_level[x] == 0; } ),
            conf_lines.end() );
        if ( conf_lines.empty() )
        {
            return false;
        }
        if ( conf_lines.size() == 1 )
        {
            GateId uip = conf_lines[0];
            int uip_val = 1 - m_values[uip];
            backtrack( 0 );
            assign_node( uip, uip_val, {} );
            conf_lines.clear();
            if ( !BCP( uip ) )
            {
                if ( !conflict() )
                {
                    return false;
                }
            }
            return true;
        }

        std::vector<bool> learn_watch;
        learn_watch.reserve( conf_lines.size() );
        for ( const auto& g : conf_lines )
        {
            learn_watch.push_back( m_values[g] == 1 );
        }
        GateId learn_gate = m_ntk.create_or_learning_gate( conf_lines, learn_watch, ii_table, learn_watch_vals );
        learning_gate_count++;
        size_t new_size = m_ntk.get_gates().size();
        if ( m_values.size() < new_size )
        {
            m_values.resize( new_size, false );
            m_assigned.resize( new_size, false );
            decision_level.resize( new_size, -1 );
            source.resize( new_size );
        }
        m_assigned[learn_gate] = true;
        m_values[learn_gate] = true;
        decision_level[learn_gate] = 0;
        source[learn_gate].clear();

        std::sort( conf_lines.begin(), conf_lines.end(),
                   [&]( GateId a, GateId b ) { return decision_level[a] > decision_level[b]; } );
        GateId uip = conf_lines[0];
        GateId second = conf_lines[1];
        int second_level = decision_level[second];
        int uip_val = !m_values[uip];
        backtrack( second_level );
        std::vector<GateId> from( conf_lines.begin() + 1, conf_lines.end() );
        assign_node( uip, uip_val, from );
        conf_lines.clear();
        if ( !BCP( uip ) )
        {
            if ( !conflict() )
            {
                return false;
            }
        }
        return true;
    }

    bool cdcl_search( std::vector<bool>& input_vals )
    {

        conf_lines.clear();

        while ( true )
        {
            clean_jnodes( mdi.back().j_nodes );

            if ( mdi.back().j_nodes.empty() )
            {
                for ( GateId pi : m_ntk.get_inputs() )
                {
                    if ( m_assigned[pi] )
                    {
                        input_vals[pi - 1] = m_values[pi];
                    }
                    else
                    {
                        input_vals[pi - 1] = 0;
                    }
                }
                return true;
            }

            GateId gate = pick_from_j_node();
            if ( gate == INVALID_GATE )
            {
                return false;
            }
            cur_level++;
            DecInfor newframe;
            newframe.dec_line = gate;
            newframe.trail_start_index = trail_node.size();
            newframe.j_nodes = mdi.back().j_nodes;
            auto saved_jnodes = newframe.j_nodes;
            mdi.push_back( std::move( newframe ) );
            assign_node( gate, false, {} );

            if ( !BCP( gate ) )
            {
                if ( !conflict() )
                {

                    return false;
                }
            }
        }
    }

    // Main SAT solving function
    bool solve( std::vector<bool>& solution )
    {
        // std::cout<<"input_num is "<<m_ntk.get_num_pis()<<std::endl;
        m_original_gate_size = m_ntk.get_gates().size();
        // std::cout << "[DEBUG] Original gate count: " << m_original_gate_size << "\n";
        solution.resize( m_ntk.get_inputs().size() );
        // print_implication_tables(di_table, ii_table);
        if ( !po_first() )
        {
            // std::cout<<"po_first is successful"<<std::endl;
            return false;
        }
        return cdcl_search( solution );
    }
};

inline std::pair<bool, std::optional<std::vector<bool>>> solve_aig( aig_ntk& ntk )
{
    aig_dpll_solver solver( ntk );
    std::vector<bool> solution;

    bool is_sat = solver.solve( solution );

    if ( is_sat )
    {
        return { true, std::move( solution ) };
    }
    else
    {
        return { false, std::nullopt };
    }
}

} // namespace cirsat

#endif // CIRSAT_AIG_DPLL_SOLVER_HPP

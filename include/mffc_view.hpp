#pragma once
#include "aig.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace cirsat {

class mffc_view
{
  public:
    using node = GateId;

    explicit mffc_view( aig_ntk const& ntk, node root, uint32_t limit = 100u )
        : _ntk( ntk ), _root( root ), _limit( limit )
    {
        _constants.push_back( 0u );
        _node_to_index.emplace( 0u, 0u );
        recompute();
    }

    uint32_t size() const
    {
        return _num_constants + _num_leaves + static_cast<uint32_t>( _inner.size() );
    }
    uint32_t num_pis() const
    {
        return _num_leaves;
    }
    uint32_t num_pos() const
    {
        return _empty ? 0u : 1u;
    }
    uint32_t num_gates() const
    {
        return static_cast<uint32_t>( _inner.size() );
    }

    bool empty() const
    {
        return _empty;
    }
    node root() const
    {
        return _root;
    }

    bool is_pi( node n ) const
    {
        return n < _is_pi.size() && _is_pi[n];
    }

    std::vector<node> const& leaves() const
    {
        return _leaves;
    }
    std::vector<node> const& gates() const
    {
        return _inner;
    }

    template <typename Fn> void foreach_pi( Fn&& fn ) const
    {
        for ( uint32_t i = 0; i < _leaves.size(); ++i )
        {
            fn( _leaves[i], i );
        }
    }

    template <typename Fn> void foreach_gate( Fn&& fn ) const
    {
        for ( uint32_t i = 0; i < _inner.size(); ++i )
        {
            fn( _inner[i], i );
        }
    }

    template <typename Fn> void foreach_node( Fn&& fn ) const
    {
        uint32_t i = 0;
        for ( auto const& n : _constants )
        {
            fn( n, i++ );
        }
        for ( auto const& n : _leaves )
        {
            fn( n, i++ );
        }
        for ( auto const& n : _inner )
        {
            fn( n, i++ );
        }
    }

    node index_to_node( uint32_t index ) const
    {
        if ( index < _num_constants )
        {
            return _constants[index];
        }
        if ( index < _num_constants + _num_leaves )
        {
            return _leaves[index - _num_constants];
        }
        return _inner[index - _num_constants - _num_leaves];
    }

    uint32_t node_to_index( node n ) const
    {
        return _node_to_index.at( n );
    }

    void update_mffc()
    {
        recompute();
    }

  private:
    static bool is_constant( node n )
    {
        return n == 0u;
    }

    void recompute()
    {
        _empty = true;
        _nodes.clear();
        _leaves.clear();
        _inner.clear();
        _topo.clear();
        _node_to_index.clear();
        _node_to_index.emplace( 0u, 0u );

        auto const& gates = _ntk.get_gates();
        _is_pi.assign( gates.size(), false );
        for ( auto const& pi : _ntk.get_inputs() )
        {
            if ( pi < _is_pi.size() )
            {
                _is_pi[pi] = true;
            }
        }

        // initialize refcounts from fanout sizes (info[0])
        _refcnt.assign( gates.size(), 0u );
        for ( uint32_t i = 0; i < gates.size(); ++i )
        {
            _refcnt[i] = gates[i].info[0];
        }

        if ( is_constant( _root ) )
        {
            _empty = false;
            _num_leaves = 0u;
            return;
        }

        if ( is_pi( _root ) )
        {
            _leaves.push_back( _root );
            _node_to_index.emplace( _root, static_cast<uint32_t>( _node_to_index.size() ) );
            _empty = false;
            _num_leaves = static_cast<uint32_t>( _leaves.size() );
            return;
        }

        if ( collect( _root ) )
        {
            _empty = false;
            compute_sets();
        }

        _num_leaves = static_cast<uint32_t>( _leaves.size() );
    }

    bool collect( node n )
    {
        if ( is_constant( n ) || is_pi( n ) )
        {
            return true;
        }

        auto const& g = _ntk.get_gates()[n];
        for ( auto const child_data : g.children )
        {
            node child = aig_ntk::data_to_index( child_data );
            _nodes.push_back( child );

            if ( child >= _refcnt.size() )
            {
                continue;
            }

            if ( _refcnt[child] > 0u )
            {
                --_refcnt[child];
            }

            if ( _refcnt[child] == 0u )
            {
                if ( _nodes.size() > _limit )
                {
                    return false;
                }
                if ( !collect( child ) )
                {
                    return false;
                }
            }
        }

        return true;
    }

    void compute_sets()
    {
        std::sort( _nodes.begin(), _nodes.end() );
        _nodes.erase( std::unique( _nodes.begin(), _nodes.end() ), _nodes.end() );

        for ( auto const& n : _nodes )
        {
            if ( is_constant( n ) )
            {
                continue;
            }

            // After decrements, _refcnt[n] > 0 indicates remaining fanout outside the MFFC
            if ( is_pi( n ) || ( n < _refcnt.size() && _refcnt[n] > 0u ) )
            {
                if ( _leaves.empty() || _leaves.back() != n )
                {
                    _leaves.push_back( n );
                }
            }
            else
            {
                if ( _inner.empty() || _inner.back() != n )
                {
                    _inner.push_back( n );
                }
            }
        }

        for ( auto const& n : _leaves )
        {
            _node_to_index.emplace( n, static_cast<uint32_t>( _node_to_index.size() ) );
        }
        for ( auto const& n : _inner )
        {
            _node_to_index.emplace( n, static_cast<uint32_t>( _node_to_index.size() ) );
        }

        // root is always part of this view (unless PI/const, handled above)
        _inner.push_back( _root );
        _node_to_index.emplace( _root, static_cast<uint32_t>( _node_to_index.size() ) );

        // topo sort internal nodes (including root)
        _colors.clear();
        _colors.resize( static_cast<uint32_t>( _node_to_index.size() ), 0u );

        // constants and leaves are permanently marked
        _colors[0u] = 2u;
        for ( auto const& l : _leaves )
        {
            _colors[_node_to_index[l]] = 2u;
        }

        topo_sort_rec( _root );
        _inner = _topo;
    }

    void topo_sort_rec( node n )
    {
        auto it = _node_to_index.find( n );
        if ( it == _node_to_index.end() )
        {
            return;
        }

        auto const idx = it->second;
        if ( _colors[idx] == 2u )
        {
            return;
        }
        if ( _colors[idx] == 1u )
        {
            return;
        }

        _colors[idx] = 1u;

        auto const& g = _ntk.get_gates()[n];
        for ( auto const child_data : g.children )
        {
            node child = aig_ntk::data_to_index( child_data );
            topo_sort_rec( child );
        }

        _colors[idx] = 2u;
        _topo.push_back( n );
    }

  private:
    aig_ntk const& _ntk;
    node _root{};
    uint32_t _limit{ 100u };

    std::vector<node> _constants{ 0u };
    std::vector<node> _nodes;
    std::vector<node> _leaves;
    std::vector<node> _inner;
    std::vector<node> _topo;

    std::vector<uint8_t> _colors;
    std::vector<uint32_t> _refcnt;
    std::vector<bool> _is_pi;

    uint32_t _num_constants{ 1u };
    uint32_t _num_leaves{ 0u };
    std::unordered_map<node, uint32_t> _node_to_index;
    bool _empty{ true };
};

} // namespace cirsat

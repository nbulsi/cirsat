#ifndef CIRSAT_AIG_HPP
#define CIRSAT_AIG_HPP

#include <string>
#include <cstdint>
#include <limits>
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>
#include <array>

#define NULL_INDEX 0xffffffff
#define HAVE_INV 0xfffffffe
#define HAVE_NO_INV 0xfffffffd

namespace cirsat {

enum class GateType
{
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR,
    XNOR,
    BUF,
    MUX,
    UNKNOWN
};

using GateId = uint32_t;
struct gate
    {
        GateType type = GateType::AND;
        std::array<uint8_t, 2u> info{0, 0}; // [0]--number of fanouts; [1]--visited flag
        std::array<GateId, 2u> children{NULL_INDEX, NULL_INDEX};
        std::vector<GateId> outputs;

        union
        {
            struct
            {
                uint32_t complement : 1;
                GateId index : 31;
            };
            uint32_t data;
        };

        gate() = default;

        gate(uint32_t index, uint32_t complement)
            : index(index), complement(complement) {}

        explicit gate(uint32_t data)
            : data(data) {}

        gate operator!() const { return gate(data ^ 1); }
        gate operator+() const { return {index, 0}; }
        gate operator-() const { return {index, 1}; }
        gate operator^(bool complement) const { return gate(data ^ (complement ? 1 : 0)); }
        bool operator==(gate const& other) const { return data == other.data; }
        bool operator!=(gate const& other) const { return data != other.data; }
        bool operator<(gate const& other) const { return data < other.data; }
    };

class aig_ntk
{
public:
    
    aig_ntk() = default;

    void create_pi()
    {
        const auto index = m_gates.size();
        m_gates.emplace_back(index, 0);
        auto& node = m_gates.back();
        node.children[0] = node.children[1] = index;
        m_inputs.push_back(index);
    }

    void create_po(const gate& s)
    {
        m_gates[s.index].info[0]++;
        m_outputs.push_back(s.index);
    }

    void create_and(const gate& a, const gate& b)
    {
        /* order inputs */
        gate left = a, right = b;
        if (left.index > right.index)
        {
            std::swap(left, right);
        }
    
        assert(left.index != right.index && "AND gate cannot have same inputs");
    
        gate node;
        node.children[0] = left.data;
        node.children[1] = right.data;
    
        const auto index = m_gates.size();
        node.index = index;
        node.complement = 0;
        m_gates.emplace_back(node);
    
        m_gates[left.index].info[0]++;
        m_gates[left.index].outputs.push_back(index);
        m_gates[right.index].info[0]++;
        m_gates[right.index].outputs.push_back(index);
    }

    gate create_not(const gate& a) const
    {
        return !a;
    }

   
    const std::vector<GateId>& get_inputs() const { return m_inputs; }
    const std::vector<GateId>& get_outputs() const { return m_outputs; }
    const std::vector<gate>& get_gates() const { return m_gates; }

    const void set_num_pis(const uint32_t& num_pis)
    {
        m_num_pis = num_pis;
        m_inputs.reserve(num_pis);
    }

    const void set_num_pos(const uint32_t& num_pos)
    {
        m_num_pos = num_pos;
        m_outputs.reserve(num_pos);
    }
    const void set_num_gates(const uint32_t& num_gates)
    {
        m_num_gates = num_gates;
        m_gates.reserve(num_gates);
    }

    const void add_gate(const gate& gate)
    {
        m_gates.push_back(gate);
    }

    uint32_t get_num_pis() const { return m_num_pis; }
    uint32_t get_num_pos() const { return m_num_pos; }
    uint32_t get_num_gates() const { return m_num_gates; }

private:

    std::vector<gate> m_gates;
    std::vector<GateId> m_inputs;
    std::vector<GateId> m_outputs;
    uint32_t m_num_pis{0u};
    uint32_t m_num_pos{0u};
    uint32_t m_num_gates{0u};
};

} // namespace cirsat

#endif // CIRSAT_AIG_HPP
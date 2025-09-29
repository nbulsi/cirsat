#ifndef CIRSAT_AIG_HPP
#define CIRSAT_AIG_HPP

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#define NULL_INDEX 0xffffffff

namespace cirsat
{

enum class GateType { AND, OR, NOT, NAND, NOR, XOR, XNOR, BUF, MUX, UNKNOWN };

using GateId = uint32_t;
using Direct_ImplicationTable = std::unordered_map<GateId, std::array<std::vector<std::pair<GateId, bool>>, 2>>;
using Indirect_ImplicationTable = std::array<std::vector<std::vector<GateId>>, 2>;
constexpr uint32_t INVALID_GATE = std::numeric_limits<uint32_t>::max();
struct Watch_Values{
    bool input1;
    bool input2;
    bool output;
};

struct gate {
    GateType type = GateType::AND;
    std::array<uint8_t, 2u> info{0, 0}; // [0]--number of fanouts; [1]--visited flag
    std::array<GateId, 2u> children{NULL_INDEX, NULL_INDEX};
    std::vector<GateId> outputs;
    float activity = 0.0f;

    union {
        struct {
            uint32_t complement : 1;
            GateId index : 31;
        };
        uint32_t data;
    };

    gate() = default;

    gate(uint32_t index, uint32_t complement) : index(index), complement(complement) {}

    explicit gate(uint32_t data) : data(data) {}

    gate operator!() const
    {
        return gate(data ^ 1);
    }
    gate operator+() const
    {
        return {index, 0};
    }
    gate operator-() const
    {
        return {index, 1};
    }
    gate operator^(bool complement) const
    {
        return gate(data ^ (complement ? 1 : 0));
    }
    bool operator==(gate const& other) const
    {
        return data == other.data;
    }
    bool operator!=(gate const& other) const
    {
        return data != other.data;
    }
    bool operator<(gate const& other) const
    {
        return data < other.data;
    }
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
        m_outputs.push_back(s.data);
    }

    void create_and(const gate& a, const gate& b)
    {
        /* order inputs */
        gate left = a, right = b;
        if (left.index > right.index) {
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
    
    const std::vector<GateId>& get_inputs() const
    {
        return m_inputs;
    }
    const std::vector<GateId>& get_outputs() const
    {
        return m_outputs;
    }
    const std::vector<gate>& get_gates() const
    {
        return m_gates;
    }

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

    uint32_t get_num_pis() const
    {
        return m_num_pis;
    }
    
    uint32_t get_num_pos() const
    {
        return m_num_pos;
    }

    uint32_t get_num_gates() const
    {
        return m_num_gates;
    }

    static GateId data_to_index(uint32_t data) 
    {
        return data >> 1;
    }

    static bool data_to_complement(uint32_t data)
    {
        return data & 1;
    }
    
    void build_implication_table(Direct_ImplicationTable& ditable,Indirect_ImplicationTable& iitable, std::unordered_map<GateId, Watch_Values>& watch_vals) const {
        size_t num_gates = m_gates.size();
        for (int val = 0; val < 2; val++) {
            iitable[val].resize(num_gates);
        }
        for (GateId id = m_inputs.size()+1; id < m_gates.size(); ++id) {
            const gate& g = m_gates[id];            
            GateId a = data_to_index(g.children[0]);
            GateId b = data_to_index(g.children[1]);
            bool a_neg = data_to_complement(g.children[0]);
            bool b_neg = data_to_complement(g.children[1]);
            watch_vals[id].input1 = !a_neg;
            watch_vals[id].input2 = !b_neg;
            watch_vals[id].output = false;

            bool is_output = false;
            GateId po_data;

            bool a_wv = watch_vals[id].input1;
            bool b_wv = watch_vals[id].input2;
            bool id_wv = watch_vals[id].output;
            // if a/b==0, then z = 0
            ditable[a][!a_wv].emplace_back(id, id_wv);
            ditable[b][!b_wv].emplace_back(id, id_wv);
            // if z==1, then a==1&b==1
            ditable[id][!id_wv].emplace_back(a, a_wv);
            ditable[id][!id_wv].emplace_back(b, b_wv);

            //indirect_implication

            iitable[a_wv][a].push_back(id);
            iitable[b_wv][b].push_back(id);
            iitable[id_wv][id].push_back(id);

        }
    }

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
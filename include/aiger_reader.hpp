/**
 * @file aiger_reader.hpp
 * @brief Template-based Parser of AIG (使用 get_gates() 等函数替代直接访问 m_gates)
 */
#include "./aig.hpp"
#include <../lib/lorina/lorina/aiger.hpp>

#ifndef CIRSAT_AIGER_READER_HPP
#define CIRSAT_AIGER_READER_HPP

namespace cirsat {

template <typename Ntk>
class aiger_reader : public lorina::aiger_reader
{
public:
    explicit aiger_reader(Ntk& ntk)
        : _ntk(ntk) {}
    ~aiger_reader()
    {
        // // Print gates and parameters for debugging
        // std::cout << "gates in _ntk.get_gates(): " << std::endl;
        // for (size_t i = 0; i < _ntk.get_gates().size(); ++i) // 使用 get_gates() 替代直接访问 m_gates
        // {
        //     const auto& gate = _ntk.get_gates()[i]; // 使用 get_gates()
        //     std::cout << "Node " << i << ": " << std::endl;
        //     std::cout << "  Data: " << gate.data << std::endl;

        //     // 打印每个节点的 children 信息
        //     std::cout << "  Children: ";
        //     for (const auto& child : gate.children)
        //     {
        //         std::cout << child << " ";
        //     }
        //     std::cout << std::endl;

        //     std::cout << "  Info:" << std::endl;
        //     std::cout << "    Fanouts: " << static_cast<int>(gate.info[0]) << std::endl;
        //     std::cout << "    Visited: " << static_cast<int>(gate.info[1]) << std::endl;
        //     for (const auto& output : gate.outputs)
        //     {
        //         std::cout << "    Output: " << output << std::endl;
        //     }
        // }

        // std::cout << "Outputs: ";
        // for (const auto& out : outputs)
        // {
        //     std::cout << "(" << std::get<0>(out) << ", " << std::get<1>(out) << ") ";
        // }
        // std::cout << std::endl;

        uint32_t output_id{0};
        for (auto out : outputs)
        {
            auto const lit = std::get<0>(out);
            auto gate = _ntk.get_gates()[lit >> 1]; // 使用 get_gates() 替代直接访问 m_gates
            if (lit & 1)
            {
                gate = _ntk.create_not(gate);
            }
            _ntk.create_po(gate);
        }
    }

    void on_header(uint64_t, uint64_t num_inputs, uint64_t num_latches, uint64_t num_outputs, uint64_t num_ands) const override
    {
        assert(num_latches == 0 && "This solver does not support latches");
        std::cout<<"num_inputs = " << num_inputs << std::endl;
        _ntk.set_num_pis(static_cast<uint32_t>(num_inputs));  
        _ntk.set_num_pos(static_cast<uint32_t>(num_outputs));
        _ntk.set_num_gates(static_cast<uint32_t>(num_ands));  
        _ntk.add_gate(gate(0));              
        /* create primary inputs (pi) */
        for (auto i = 0u; i < num_inputs; ++i)
        {
            _ntk.create_pi();
        }
    }

    void on_and(unsigned index, unsigned left_lit, unsigned right_lit) const override
    {
        (void)index;
        assert(_ntk.get_gates().size() == index); // 使用 get_gates()

        auto left = _ntk.get_gates()[left_lit >> 1]; // 使用 get_gates()
        if (left_lit & 1)
        {
            left = _ntk.create_not(left);
        }

        auto right = _ntk.get_gates()[right_lit >> 1]; // 使用 get_gates()
        if (right_lit & 1)
        {
            right = _ntk.create_not(right);
        }

        _ntk.create_and(left, right);
    }

    void on_output(unsigned index, unsigned lit) const override
    {
        (void)index;
        assert(index == _ntk.get_outputs().size()); // 使用 get_outputs()
        outputs.emplace_back(lit, "");
    }

private:
    Ntk& _ntk;
    mutable std::vector<std::tuple<unsigned, std::string>> outputs;
};

} // namespace cirsat

#endif // CIRSAT_AIGER_READER_HPP
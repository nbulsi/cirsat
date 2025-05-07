/**
 * @file aiger_reader.hpp
 * @brief Template-based Parser of AIG
 * @author Haonan Wei
 * @author Zhufei Chu 
 */

#include "aig.hpp"
#include <lorina/aiger.hpp>

#ifndef CIRSAT_AIGER_READER_HPP
#define CIRSAT_AIGER_READER_HPP

namespace cirsat
{

template <typename Ntk> class aiger_reader : public lorina::aiger_reader
{
  public:
    explicit aiger_reader(Ntk& ntk) : _ntk(ntk) {}
    // ~aiger_reader()
    // {
    //     uint32_t output_id{0};
    //     for (auto out : outputs) {
    //         auto const lit = std::get<0>(out);
    //         auto gate = _ntk.get_gates()[lit >> 1];
    //         if (lit & 1) {
    //             gate = _ntk.create_not(gate);
    //         }
    //         _ntk.create_po(gate);
    //         //printf("output size %ld\n", _ntk.get_outputs().size());
    //     }
    // }
    void process_outputs() const {
        for (auto out : outputs) {
            auto const lit = std::get<0>(out);
            auto gate = _ntk.get_gates()[lit >> 1];
            if (lit & 1) {
                gate = _ntk.create_not(gate);
            }
            _ntk.create_po(gate);
        }
    }

    void on_header(uint64_t, uint64_t num_inputs, uint64_t num_latches, uint64_t num_outputs,
                   uint64_t num_ands) const override
    {
        assert(num_latches == 0 && "This solver does not support latches");
        
        _ntk.set_num_pis(static_cast<uint32_t>(num_inputs));
        _ntk.set_num_pos(static_cast<uint32_t>(num_outputs));
        _ntk.set_num_gates(static_cast<uint32_t>(num_ands));
        _ntk.add_gate(gate(0));

        /* create primary inputs (pi) */
        for (auto i = 0u; i < num_inputs; ++i) {
            _ntk.create_pi();
        }
    }

    void on_and(unsigned index, unsigned left_lit, unsigned right_lit) const override
    {
        (void)index;
        assert(_ntk.get_gates().size() == index);

        auto left = _ntk.get_gates()[left_lit >> 1];
        if (left_lit & 1) {
            left = _ntk.create_not(left);
        }

        auto right = _ntk.get_gates()[right_lit >> 1];
        if (right_lit & 1) {
            right = _ntk.create_not(right);
        }

        _ntk.create_and(left, right);
        if (_ntk.get_gates().size() == _ntk.get_num_gates()+_ntk.get_num_pis() + 1) {
            process_outputs();
        }
    }

    void on_output(unsigned index, unsigned lit) const override
    {
        (void)index;
        
        outputs.emplace_back(lit, "");
    }

  private:
    Ntk& _ntk;
    mutable std::vector<std::tuple<unsigned, std::string>> outputs;
};

} // namespace cirsat

#endif // CIRSAT_AIGER_READER_HPP
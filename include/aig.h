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

#ifndef AIG_H
#define AIG_H

#define NULL_INDEX 0xffffffff
#define HAVE_INV 0xfffffffe
#define HAVE_NO_INV 0xfffffffd

using GateId = uint32_t;

class Gate
{
public:

	const bool is_input() const { return m_inputs[0] == NULL_INDEX; }
	const bool is_output() const { return m_outputs.size() == 1 && (m_outputs[0] == HAVE_INV || m_outputs[0] == HAVE_NO_INV); }

	const uint32_t &get_id() const { return m_id; }
	uint32_t &id() { return m_id; }

	const std::array<GateId, 2u> &get_inputs() const { return m_inputs; }
	std::array<GateId, 2u> &inputs() { return m_inputs; }

	const std::vector<GateId> &get_outputs() const { return m_outputs; }
	std::vector<GateId> &outputs() { return m_outputs; }

private:
	uint32_t m_id{ NULL_INDEX };
	std::array<GateId, 2u> m_inputs{NULL_INDEX, NULL_INDEX}; //相连的 节点id * 2 + int(是否有反相器)
	std::vector<GateId> m_outputs;                           //除原始输出以外，都没有反向器
};

class AigGraph
{
public:
	GateId add_input(const uint32_t &temp);
	GateId add_output(const uint32_t &temp);
	GateId add_gate(const uint32_t &output, const uint32_t &input1, const uint32_t &input2);
	
public:
	const Gate& get_gate(GateId n) const { return m_gates[n];}
	Gate& gate(GateId n) { return m_gates[n];}

	const size_t num_fanin(GateId n) const { return m_gates[n].get_inputs().size(); }
	const size_t num_fanout(GateId n) const { return m_gates[n].get_outputs().size(); }

	const uint32_t get_numG() { return m_gates.size(); }
	const uint32_t get_numPI() { return m_inputs.size(); }
	const uint32_t get_numPO() { return m_outputs.size(); }

	const std::vector<GateId> &get_inputs() const { return m_inputs; };
	std::vector<GateId> &inputs() { return m_inputs; }

	const std::vector<GateId> &get_outputs() const { return m_outputs; };
	std::vector<GateId> &outputs() { return m_outputs; }

	const std::vector<Gate> &get_gates() const { return m_gates; }
	std::vector<Gate> &gates() { return m_gates; };

	void printf_graph();

private:

private:
	uint32_t m_gates_num { 0u };
	uint32_t m_pis_num { 0u };
	uint32_t m_pos_num { 0u };

	std::vector<Gate> m_gates;
	std::vector<GateId> m_inputs;
	std::vector<GateId> m_outputs;
};
#endif
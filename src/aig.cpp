#include "../include/aig.h"


GateId AigGraph::add_input(const uint32_t &temp)
{
  assert(temp % 2 == 0 );
	GateId gate_id = temp / 2 - 1;
	m_inputs.push_back(gate_id);
	return gate_id;
}

GateId AigGraph::add_output(const uint32_t &temp)
{
	GateId gate_id = temp / 2 - 1;
  if(temp % 2 == 0)
    m_gates[gate_id].outputs().push_back(HAVE_NO_INV);
  else 
    m_gates[gate_id].outputs().push_back(HAVE_INV);
	m_outputs.push_back(gate_id);
	return gate_id;
}

GateId AigGraph::add_gate(const uint32_t &output, const uint32_t &input1, const uint32_t &input2)
{
  assert(output % 2 == 0 );

	GateId gate_id = output / 2 - 1;
	Gate& gate = m_gates[gate_id];

  GateId i1 = input1 / 2 - 1;
  gate.inputs()[0] = input1 - 2;
  m_gates[i1].outputs().push_back(gate_id);

  GateId i2 = input2 / 2 - 1;
  gate.inputs()[1] = input2 - 2;
  m_gates[i2].outputs().push_back(gate_id);

	return gate_id;
}

void AigGraph::printf_graph()
{
  std::cout << "aag " << get_numG() << " " << get_numPI() << " " << "0 " << get_numPO() << " " 
            <<  get_numG() - get_numPI() << std::endl;
  for(const auto& pi : m_inputs)
    std::cout << (m_gates[pi].get_id() + 1) * 2 << std::endl;
  for(const auto& po : m_outputs)
    std::cout << (m_gates[po].get_id() + 1) * 2 + int(m_gates[po].get_outputs()[0] == HAVE_INV) << std::endl;   
  int fan0 = 0;
  for(const auto& gate : m_gates)
  {
    if(gate.is_input() && gate.get_outputs().size() == 0)
    {
      fan0++;
    }
  }
  std::cout <<"fan0 = "<<fan0<< std::endl;
  for(const auto& po : m_outputs)
  {
    std::cout << m_gates[po].get_inputs().size() << std::endl;
  }
}
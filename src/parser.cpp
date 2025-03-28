#include "../include/parser.h"
#include "../include/solver.h"
bool Parser::parse(std::istream& is, AigGraph& graph)
{
	int line_ctr = 0;
  int inputs_num;
  int outputs_num;
  int num;
  int and_num;
	for (std::string line; std::getline(is, line, '\n');) 
  {
		++line_ctr;
    if(line.find("aag") != std::string::npos)
    {
      std::vector<std::string> temp = m_split(line, " ");
      and_num = std::stoi(temp[5]);
      if (and_num == 0) return false;
      num = std::stoi(temp[1]);
      inputs_num = std::stoi(temp[2]);
      outputs_num = std::stoi(temp[4]);
      //开辟空间、制作id
      graph.gates().resize(num, Gate());
      for(uint32_t i = 0; i < graph.gates().size(); i++)
        graph.gates()[i].id() = i;
      graph.inputs().reserve(inputs_num);
      graph.outputs().reserve(outputs_num);
      continue;
    }
    if(line_ctr > 1 && line_ctr <= inputs_num + 1)
    {
      graph.add_input(std::stoi(line));
      continue;
    }
    if(line_ctr > inputs_num + 1 && line_ctr <= inputs_num + outputs_num + 1)
    {
      graph.add_output(std::stoi(line));
      continue;
    }
    if(line_ctr <= num + outputs_num + 1)
    {
      std::vector<std::string> gate = m_split(line, " ");
      graph.add_gate(std::stoi(gate[0]), std::stoi(gate[1]), std::stoi(gate[2]));
      continue;
    }
	}
	return true;
}
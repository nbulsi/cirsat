#include "aig.h"
#include "myfunction.h"

#ifndef PARSER_H
#define PARSER_H

class Parser
{
public:
	bool parse(std::istream& is, AigGraph& graph);
};




#endif
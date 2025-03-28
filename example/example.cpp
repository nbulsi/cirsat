#include "../include/parser.h"
#include "../include/Time.h"
#include "../include/solver.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error

int main()
{
    try {
        std::ifstream ifs("../../example/SAT/9sym.aag");
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open file: ../../example/SAT/9sym.aag");
        }

        AigGraph aig;
        Parser parser;
        if (!parser.parse(ifs, aig)) {
            throw std::runtime_error("Failed to parse file: ../../example/SAT/9sym.aag");
        }

        solver m_solver(aig);
        int ConfLimit = 1000;
        int Verbose = 1;
        m_solver.run(ConfLimit, Verbose);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

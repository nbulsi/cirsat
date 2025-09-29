/**
 * @file cirsat.cpp
 * @brief Main entry for the cirsat SAT solver
 *
 * Copyright (c) 2023- Zhufei Chu, Ningbo University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "solver.hpp"
#include "aig.hpp"
#include "aig_dpll_solver.hpp"
#include "aiger_reader.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

void printUsage()
{
    std::cout << "Usage: cirsat [command] [options]\n"
              << "\nCommands:\n"
              << "  solve <circuit_file>    Solve the circuit specified in the file\n"
              << "\nOptions:\n"
              << "  -h, --help             Show this help message\n"
              << "  -v, --version          Show version information\n"
              << "  --verbose              Enable verbose output\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string arg = argv[1];

    // Handle help option
    if (arg == "-h" || arg == "--help") {
        printUsage();
        return 0;
    }

    // Handle version option
    if (arg == "-v" || arg == "--version") {
        std::cout << "cirsat version 1.0.0\n";
        return 0;
    }

    if (arg == "solve") {
        if (argc < 3) {
            std::cout << "Error: No circuit file specified\n";
            return 1;
        }

        bool verbose = false;
        for (int i = 3; i < argc; i++) {
            std::string option = argv[i];
            if (option == "--verbose") {
                verbose = true;
            }
        }

        if (verbose) {
            std::cout << "Processing circuit file: " << argv[2] << std::endl;
        }

        // read aiger file and construct aig_ntk network
        cirsat::aig_ntk network;
        cirsat::aiger_reader<cirsat::aig_ntk> reader(network);
        
        std::ifstream file(argv[2]);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open file " << argv[2] << std::endl;
            return 1;
        }

        auto result = lorina::read_aiger(file, reader);
        file.close();

        if (result != lorina::return_code::success) {
            std::cout << "Error: Failed to parse AIGER file" << std::endl;
            return 1;
        }
        
        // solve aig_ntk network
        auto [is_sat, solution] = cirsat::solve_aig(network);
        
        if (is_sat) {
            std::cout << "SAT\n";
            if (verbose && solution) {
                std::cout << "Solution:\n";
                for (size_t i = 0; i < solution->size(); ++i) {
                    std::cout << "Input " << i << ": " << ((*solution)[i] ? "1" : "0") << "\n";
                }
            }
        } else {
            std::cout << "UNSAT\n";
        }

        return 0;
    }

    std::cout << "Unknown command: " << arg << std::endl;
    printUsage();
    return 1;
}

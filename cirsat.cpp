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
#include "lut_network.hpp"
#include "bench_reader.hpp"
#include "lut_dpll_solver.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

void printUsage()
{
    std::cout << "Usage: cirsat [command] [options]\n"
              << "\nCommands:\n"
              << "  solve [options] <circuit_file>    Solve the circuit specified in the file\n"
              << "  analyze <bench_file>              Analyze the LUT circuit without solving\n"
              << "\nOptions:\n"
              << "  -h, --help             Show this help message\n"
              << "  -l, --lut              Solve LUT network (bench format)\n"
              << "  -v, --verbose          Enable verbose output\n"
              << "  --debug                Enable debug output with detailed trace (LUT only)\n";
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

    if (arg == "solve") {
        if (argc < 3) {
            std::cout << "Error: No circuit file specified\n";
            return 1;
        }

        bool verbose = false;
        bool debug = false;
        bool use_lut = false;
        std::string circuit_file;
        
        // Parse all arguments after "solve"
        for (int i = 2; i < argc; i++) {
            std::string option = argv[i];
            if (option == "-v" || option == "--verbose") {
                verbose = true;
            } else if (option == "--debug") {
                debug = true;
                verbose = true;  // debug implies verbose
            } else if (option == "-l" || option == "--lut") {
                use_lut = true;
            } else {
                // This should be the circuit file
                circuit_file = option;
            }
        }

        if (circuit_file.empty()) {
            std::cout << "Error: No circuit file specified\n";
            return 1;
        }

        if (verbose) {
            std::cout << "Processing circuit file: " << circuit_file << std::endl;
        }

        // Solve LUT network (bench format)
        if (use_lut) {
            // Read bench file and construct lut_network
            cirsat::lut_network network;
            cirsat::bench_reader<cirsat::lut_network> reader(network);
            
            std::ifstream file(circuit_file);
            if (!file.is_open()) {
                std::cout << "Error: Cannot open file " << circuit_file << std::endl;
                return 1;
            }

            bool parse_result = reader.parse(file);
            file.close();

            if (!parse_result) {
                std::cout << "Error: Failed to parse bench file" << std::endl;
                return 1;
            }

            if (verbose) {
                std::cout << "\nNetwork parsed successfully!\n";
                network.print_stats();
                network.print_network();
                
                std::cout << "\nPerforming network analysis...\n";
                network.compute_levels();
                network.print_detailed_analysis();
                
                std::cout << "\n" << std::string(50, '=') << std::endl;
                std::cout << "Starting SAT solver..." << std::endl;
                std::cout << std::string(50, '=') << "\n" << std::endl;
            }

            cirsat::lut_dpll_solver::solve_mode mode = 
                debug ? cirsat::lut_dpll_solver::solve_mode::DEBUG :
                verbose ? cirsat::lut_dpll_solver::solve_mode::VERBOSE :
                cirsat::lut_dpll_solver::solve_mode::NORMAL;

            cirsat::lut_dpll_solver solver(network, mode);
            bool is_sat = solver.solve();
            
            if (is_sat) {
                std::cout << "\nSAT\n";
                if (verbose) {
                    solver.print_solution();
                    solver.print_stats();
                }
            } else {
                std::cout << "\nUNSAT\n";
                if (verbose) {
                    solver.print_stats();
                }
            }
        }
        // Solve AIG network (aiger format)
        else {
            // read aiger file and construct aig_ntk network
            cirsat::aig_ntk network;
            cirsat::aiger_reader<cirsat::aig_ntk> reader(network);
            
            std::ifstream file(circuit_file);
            if (!file.is_open()) {
                std::cout << "Error: Cannot open file " << circuit_file << std::endl;
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
        }

        return 0;
    }

    if (arg == "analyze") {
        if (argc < 3) {
            std::cout << "Error: No bench file specified\n";
            return 1;
        }

        bool verbose = false;
        for (int i = 3; i < argc; i++) {
            std::string option = argv[i];
            if (option == "-v" || option == "--verbose") {
                verbose = true;
            }
        }

        if (verbose) {
            std::cout << "Processing bench file: " << argv[2] << std::endl;
        }

        // Read bench file and construct lut_network
        cirsat::lut_network network;
        cirsat::bench_reader<cirsat::lut_network> reader(network);
        
        std::ifstream file(argv[2]);
        if (!file.is_open()) {
            std::cout << "Error: Cannot open file " << argv[2] << std::endl;
            return 1;
        }

        bool parse_result = reader.parse(file);
        file.close();

        if (!parse_result) {
            std::cout << "Error: Failed to parse bench file" << std::endl;
            return 1;
        }

        std::cout << "\nNetwork parsed successfully!\n";
        network.print_stats();
        network.print_network();
        
        std::cout << "\nPerforming network analysis...\n";
        network.compute_levels();
        network.print_detailed_analysis();
        
        std::cout << "\nAnalysis complete (solve skipped)\n";

        return 0;
    }

    std::cout << "Unknown command: " << arg << std::endl;
    printUsage();
    return 1;
}

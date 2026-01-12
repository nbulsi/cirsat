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

#include "aig.hpp"
#include "mffc_view.hpp"
#include "solver.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

static void printUsage()
{
    std::cout << "Usage: cirsat [command] [options]\n"
              << "\nCommands:\n"
              << "  solve <circuit_file>    Solve the circuit specified in the file\n"
              << "\nOptions:\n"
              << "  -h, --help             Show this help message\n"
              << "  -v, --version          Show version information\n"
              << "  --limit <N>            Max nodes traversed when collecting an MFFC (default 100)\n";
}

int main( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        printUsage();
        return 1;
    }

    std::string arg = argv[1];

    // Handle help option
    if ( arg == "-h" || arg == "--help" )
    {
        printUsage();
        return 0;
    }

    // Handle version option
    if ( arg == "-v" || arg == "--version" )
    {
        std::cout << "cirsat version 1.0.0\n";
        return 0;
    }

    if ( arg == "solve" )
    {
        if ( argc < 3 )
        {
            std::cout << "Error: No circuit file specified\n";
            return 1;
        }

        bool verbose = false;
        uint32_t limit = 100u;
        for ( int i = 3; i < argc; i++ )
        {
            std::string option = argv[i];
            if ( option == "--verbose" )
            {
                verbose = true;
            }
            else if ( option == "--limit" && i + 1 < argc )
            {
                limit = static_cast<uint32_t>( std::stoul( argv[++i] ) );
            }
        }

        if ( verbose )
        {
            std::cout << "Processing circuit file: " << argv[2] << std::endl;
        }

        cirsat::Solver solver;
        if ( !solver.load_aiger( argv[2] ) )
        {
            std::cout << "Error: Cannot open or parse file " << argv[2] << std::endl;
            return 1;
        }

        auto [is_sat, solution] = solver.solve();
        if ( is_sat )
        {
            std::cout << "SAT\n";
            if ( verbose && solution )
            {
                std::cout << "Solution:\n";
                for ( size_t i = 0; i < solution->size(); ++i )
                {
                    std::cout << "Input " << i << ": " << ( ( *solution )[i] ? "1" : "0" ) << "\n";
                }
            }
        }
        else
        {
            std::cout << "UNSAT\n";
        }

        if ( verbose )
        {
            // Enumerate MFFCs
            const auto& network = solver.network();
            const auto& gates = network.get_gates();
            const auto& inputs = network.get_inputs();
            const auto first_internal = static_cast<cirsat::GateId>( inputs.size() + 1u );
            uint32_t matched = 0u;

            for ( cirsat::GateId root = first_internal; root < gates.size(); ++root )
            {
                cirsat::mffc_view view{ network, root, limit };
                if ( view.empty() )
                {
                    continue;
                }

                ++matched;

                if ( verbose )
                {
                    std::cout << "[mffc] root=g" << root << " size=" << view.size() << " leaves=" << view.num_pis()
                              << " gates=" << view.num_gates() << "\n";
                }
            }

            std::cout << "[mffc] count=" << matched << "\n";
        }
        return 0;
    }

    std::cout << "Unknown command: " << arg << std::endl;
    printUsage();
    return 1;
}

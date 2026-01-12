#include "solver.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

int main()
{
    // Create a temporary AIGER file for demonstration
    // Represents a simple AND gate: Output = Input1 AND Input2
    std::string filename = "simple_test.aag";
    {
        std::ofstream file( filename );
        if ( !file )
        {
            std::cerr << "Error creating temporary file" << std::endl;
            return 1;
        }
        // Header: M=3, I=2, L=0, O=1, A=1
        // Variables: 1 (input), 2 (input), 3 (and gate)
        // Literals: 2, 4, 6
        file << "aag 3 2 0 1 1\n";
        file << "2\n";     // Input 1 (variable 1)
        file << "4\n";     // Input 2 (variable 2)
        file << "6\n";     // Output 1 (variable 3)
        file << "6 2 4\n"; // AND gate: 3 = 1 AND 2
        file.close();
    }

    cirsat::Solver solver;

    std::cout << "Loading circuit from " << filename << "..." << std::endl;
    if ( !solver.load_aiger( filename ) )
    {
        std::cerr << "Failed to load circuit." << std::endl;
        std::remove( filename.c_str() );
        return 1;
    }

    std::cout << "Solving circuit..." << std::endl;
    auto [is_sat, solution] = solver.solve();

    std::cout << "Result: " << ( is_sat ? "SAT" : "UNSAT" ) << std::endl;

    if ( is_sat && solution )
    {
        std::cout << "Assignment:" << std::endl;
        const auto& assignment = *solution;
        for ( size_t i = 0; i < assignment.size(); ++i )
        {
            std::cout << "Input " << i << ": " << ( assignment[i] ? "1" : "0" ) << std::endl;
        }
    }

    // Cleanup
    std::remove( filename.c_str() );

    return 0;
}

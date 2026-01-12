// #define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "solver.hpp"

#include "catch2/catch.hpp"
#include "solver.hpp"
#include <cstdio>
#include <fstream>

TEST_CASE( "Solver Basic Tests", "[solver]" )
{
    cirsat::Solver solver;

    SECTION( "Simple Buffer AIG (SAT)" )
    {
        // Create a temporary AAG file: Input 1 -> Output
        // aag M I L O A
        // M=1, I=1, L=0, O=1, A=0
        std::ofstream temp( "temp_sat.aag" );
        temp << "aag 1 1 0 1 0\n2\n2\n";
        temp.close();

        bool loaded = solver.load_aiger( "temp_sat.aag" );
        REQUIRE( loaded == true );

        auto [is_sat, solution] = solver.solve();
        REQUIRE( is_sat == true );
        REQUIRE( solution.has_value() );
        REQUIRE( solution->size() == 1 );
        REQUIRE( ( *solution )[0] == true ); // Input must be 1 for output to be 1

        std::remove( "temp_sat.aag" );
    }

    SECTION( "Constant False Output (UNSAT)" )
    {
        // Output wired to constant 0
        std::ofstream temp( "temp_unsat.aag" );
        temp << "aag 0 0 0 1 0\n0\n";
        temp.close();

        bool loaded = solver.load_aiger( "temp_unsat.aag" );
        REQUIRE( loaded == true );

        auto [is_sat, solution] = solver.solve();
        REQUIRE( is_sat == false );

        std::remove( "temp_unsat.aag" );
    }
}
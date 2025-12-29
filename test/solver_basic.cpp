// #define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "solver.hpp"

TEST_CASE("Solver Basic Tests", "[solver]")
{
    cirsat::Solver solver;

    SECTION("Simple AND Gate Test")
    {
        solver.addGate("AND", 1, 2);
        std::vector<bool> inputs = {true, true};

        REQUIRE(solver.solve(inputs) == true);
    }

    SECTION("Invalid Input Test")
    {
        solver.addGate("AND", 1, 2);
        std::vector<bool> inputs = {true, false};

        REQUIRE(solver.solve(inputs) == true);
    }
}

TEST_CASE("Solver Gate Addition", "[solver]")
{
    cirsat::Solver solver;

    SECTION("Multiple Gates")
    {
        REQUIRE_NOTHROW(solver.addGate("AND", 1, 2));
        REQUIRE_NOTHROW(solver.addGate("OR", 2, 3));
    }
}
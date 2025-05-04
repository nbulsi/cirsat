/**
 * @file test.cpp
 * @brief Test of parser
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */
#define CATCH_CONFIG_MAIN
#include "aiger_reader.hpp"
#include "aig.hpp"
#include <catch.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>

namespace cirsat
{

TEST_CASE("read and parse local AIGER file", "[aiger_reader]")
{
    auto start_time = std::chrono::high_resolution_clock::now();
    aig_ntk network;

    aiger_reader<aig_ntk> reader(network);

    std::string file_path = "../benchmarks/aiger/c6288.aiger";

    std::ifstream file(file_path);
    REQUIRE(file.is_open());

    auto const result = lorina::read_aiger(file, reader);
    CHECK(result == lorina::return_code::success);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double duration_in_ms = duration.count() / 1000.0;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Parsing and solver initialization took " << duration_in_ms << " ms." << std::endl;
    CHECK(network.get_num_pis() == 32);
    CHECK(network.get_num_pos() == 1);
    CHECK(network.get_num_gates() == 4073);
}

} // namespace cirsat

/**
 * @file aiger_reader.cpp
 * @brief Test of AIGER file parser
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#define CATCH_CONFIG_MAIN
#include "aiger_reader.hpp"
#include "aig.hpp"
#include <catch.hpp>
#include <fstream>
#include <iostream> 

namespace cirsat
{

TEST_CASE("read and parse local AIGER file", "[aiger_reader]")
{
    aig_ntk network;

    aiger_reader<aig_ntk> reader(network);

    std::string file_path = "../benchmarks/aiger/c6288.aiger";

    std::ifstream file(file_path);
    REQUIRE(file.is_open());

    auto const result = lorina::read_aiger(file, reader);
    file.close();
    
    CHECK(result == lorina::return_code::success);

    uint32_t num_pis = 0;
    for (const auto& pi : network.get_inputs()) 
    {
        num_pis++;
    }
    CHECK(num_pis == 32);
    CHECK(network.get_num_pis() == 32);

    uint32_t num_gates = 0;
    for (const auto& gate : network.get_gates())
    {
        num_gates++;
    }
    CHECK(num_gates == 4106);
    CHECK(network.get_num_gates() == 4073); //num_pis + num_pos + num_gates
    CHECK(network.get_num_pos() == 1);
}

} // namespace cirsat

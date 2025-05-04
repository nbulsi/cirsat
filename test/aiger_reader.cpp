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
    
    CHECK(result == lorina::return_code::success);
    CHECK(network.get_num_pis() == 32);
    CHECK(network.get_num_pos() == 1);
    CHECK(network.get_num_gates() == 4073);
    CHECK(network.get_aig_size() == 4106);

}

} // namespace cirsat

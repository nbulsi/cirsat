/**
 * @file test.cpp
 * @brief Test of parser
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */
#define CATCH_CONFIG_MAIN
#include "aiger_reader.hpp"
#include "aig.hpp"
#include <iomanip>
#include <catch.hpp>
#include <fstream>
#include <chrono> 

namespace cirsat {
 
TEST_CASE("read and parse local AIGER file", "[aiger_reader]")
{
    auto start_time = std::chrono::high_resolution_clock::now();
    // 创建 aig_ntk 实例
    aig_ntk network;
 
    // 创建 aiger_reader 实例
    aiger_reader<aig_ntk> reader(network);
 
    // 指定本地 AIGER 文件路径
    //std::string file_path = "/home/whn/cirsat/benchmarks/aiger/c17_miter.aiger"; // 替换为您的本地文件路径
    std::string file_path = "/home/whn/cirsat/benchmarks/aiger/c6288.aiger"; // 替换为您的本地文件路径
    
     // 打开文件并解析
    std::ifstream file(file_path);
    REQUIRE(file.is_open()); // 确保文件成功打开
 
    auto const result = lorina::read_aiger(file, reader);
    CHECK(result == lorina::return_code::success);
    auto end_time = std::chrono::high_resolution_clock::now();

    // 计算时间差并转换为毫秒（带小数点后三位）
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double duration_in_ms = duration.count() / 1000.0;

    // 输出时间，保留小数点后三位
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Parsing and solver initialization took " << duration_in_ms << " ms." << std::endl;
    // 验证解析结果（根据您的网络结构进行调整）
    CHECK(network.get_num_pis() > 0); // 检查是否有输入
    CHECK(network.get_num_pos() > 0); // 检查是否有输出
    CHECK(network.get_num_gates() > 0); // 检查是否有逻辑门
 
    // 示例：打印网络的输入、输出和门的数量
    std::cout << "Number of PIs: " << network.get_num_pis() << std::endl;
    std::cout << "Number of POs: " << network.get_num_pos() << std::endl;
    std::cout << "Number of gates: " << network.get_num_gates() << std::endl;
}
 
} // namespace cirsat
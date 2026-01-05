/**
 * @file solver.cpp
 * @brief Implementation of circuit-based SAT solver
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#include "solver.hpp"
#include "aig.hpp"
#include "aig_dpll_solver.hpp"
#include "aiger_reader.hpp"
#include <fstream>
#include <lorina/aiger.hpp>

namespace cirsat
{

struct Solver::Impl {
    aig_ntk network;
};

Solver::Solver() : pimpl(new Impl()) {}

Solver::~Solver()
{
    delete pimpl;
}

bool Solver::load_aiger(const std::string& filename)
{
    cirsat::aiger_reader<cirsat::aig_ntk> reader(pimpl->network);
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    auto result = lorina::read_aiger(file, reader);
    file.close();

    return result == lorina::return_code::success;
}

aig_ntk const& Solver::network() const
{
    return pimpl->network;
}

std::pair<bool, std::optional<std::vector<bool>>> Solver::solve()
{
    return cirsat::solve_aig(pimpl->network);
}

} // namespace cirsat
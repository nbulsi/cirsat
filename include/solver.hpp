/**
 * @file solver.hpp
 * @brief Circuit-based SAT solver main interface
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_SOLVER_HPP
#define CIRSAT_SOLVER_HPP

#include <string>
#include <vector>

namespace cirsat
{

class Solver
{
  public:
    Solver();
    ~Solver();

    bool solve(const std::vector<bool>& circuit);
    void addGate(const std::string& type, int input1, int input2);

  private:
    struct Impl;
    Impl* pimpl;
};

} // namespace cirsat

#endif // CIRSAT_SOLVER_HPP
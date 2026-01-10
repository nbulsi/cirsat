/**
 * @file solver.hpp
 * @brief Circuit-based SAT solver main interface
 * @copyright Copyright (c) 2023- Zhufei Chu, Ningbo University. MIT License.
 */

#ifndef CIRSAT_SOLVER_HPP
#define CIRSAT_SOLVER_HPP

#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace cirsat
{
class aig_ntk; // Forward declaration

class Solver
{
  public:
    Solver();
    ~Solver();

    // Load circuit from AIGER file
    bool load_aiger(const std::string& filename);

    // Get the network
    aig_ntk const& network() const;

    // Solve the loaded circuit
    std::pair<bool, std::optional<std::vector<bool>>> solve();

  private:
    struct Impl;
    Impl* pimpl;
};

} // namespace cirsat

#endif // CIRSAT_SOLVER_HPP
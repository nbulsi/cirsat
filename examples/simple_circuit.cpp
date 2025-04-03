#include "solver.hpp"
#include <iostream>

int main()
{
    cirsat::Solver solver;

    // 创建一个简单的电路
    solver.addGate("AND", 1, 2);

    std::vector<bool> circuit = {true, false};
    bool result = solver.solve(circuit);

    std::cout << "Circuit is " << (result ? "satisfiable" : "unsatisfiable") << std::endl;

    return 0;
}

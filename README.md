# cirsat
A circuit-based Boolean satisfiability (SAT) solver
[Read the full documentation.](https://cirsat-tool.readthedocs.io/en/latest/)

Typical SAT solvers are based on conjunctive normal form (CNF) descriptions.
Given a circuit represented by a directed acyclic graph (DAG), such as
AND-Inverter Graph (AIG) and Boolean chains, *cirsat* is based on DAG-based
logic networks instead of CNF-based formats. The main motivation is to boost
electronic design automation (EDA), especially for applications in logic synthesis.

## Building from Source

```bash
# Clone the repository
git clone https://github.com/zfchu/cirsat.git

# Enter the project directory
cd cirsat

# Create and enter build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Enable test
cmake -DBUILD_TESTING=ON ..
make
```

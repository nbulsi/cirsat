# cirsat
A circuit-based Boolean satisfiability (SAT) solver

Typical SAT solvers are based on conjunctive normal form (CNF) descriptions.
Given a circuit represented by a directed acylic graph (DAG), such as
AND-Inverter Graph (AIG) and Boolean chains, *cirsat* is based on DAG-based
logic networks instead of CNF-based formats. The main motivation is to boost
electronic desigon automation (EDA), especially for applications in logic synthesis.

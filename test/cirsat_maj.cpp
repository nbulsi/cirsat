#include <catch.hpp>
#include "../include/aig.h"
#include "../include/parser.h"
#include "../include/solver.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept> // For std::runtime_error

struct AigGraphAndSolver 
{  
    AigGraph m_aig;  
    solver m_solver;  
    AigGraphAndSolver(const AigGraph& a, const solver& s):m_aig(a), m_solver(s){}
};  

AigGraphAndSolver initial_cirsat_maj()
{
  std::ifstream ifs("../../test/maj.aag");
  AigGraph aig;
  Parser parser;
  if (!parser.parse(ifs, aig))
    throw std::runtime_error("Failed to parse file: ../../test/maj.aag");

  CHECK( aig.get_inputs().size() == 3 );
  CHECK( aig.get_outputs().size() == 1 );
  CHECK( (aig.get_gates().size() - aig.get_inputs().size()) == 6 );

  solver solve(aig);
  AigGraphAndSolver result(aig, solve);
  return result; 
}

AigGraphAndSolver run_cirsat_maj()
{
  std::ifstream ifs("../../test/maj.aag");
  AigGraph aig;
  Parser parser;
  if (!parser.parse(ifs, aig))
    throw std::runtime_error("Failed to parse file: ../../test/maj.aag");

  CHECK( aig.get_inputs().size() == 3 );
  CHECK( aig.get_outputs().size() == 1 );
  CHECK( (aig.get_gates().size() - aig.get_inputs().size()) == 6 );

  solver solve(aig);
  int ConfLimit = 10000;
  int Verbose = 0;
  solve.run(ConfLimit, Verbose);

  AigGraphAndSolver result(aig, solve);
  return result; 
}

TEST_CASE( "parse function", "[maj]" )
{
  AigGraph aig = initial_cirsat_maj().m_aig;
  //PIs encoding test
  CHECK( aig.get_inputs()[0] == (2-2)/2 );
  CHECK( aig.get_inputs()[1] == (4-2)/2 );
  CHECK( aig.get_inputs()[2] == (6-2)/2 );
  //POs encoding test
  CHECK( aig.get_outputs()[0] == (19-2)/2 );
  //gates encoding test
  CHECK( aig.get_gates()[3].get_id() == (8-2)/2 );
  CHECK( aig.get_gates()[4].get_id() == (10-2)/2 );
  CHECK( aig.get_gates()[5].get_id() == (12-2)/2 );
  CHECK( aig.get_gates()[6].get_id() == (14-2)/2 );
  CHECK( aig.get_gates()[7].get_id() == (16-2)/2 );
  CHECK( aig.get_gates()[8].get_id() == (18-2)/2 );

}

TEST_CASE( "solver constructor", "[maj]" )
{
  solver solve = initial_cirsat_maj().m_solver;
  std::vector<GateInfo> gateinfo = solve.get_m_val_info();
  CHECK( gateinfo.size() == 9 );
  //电路中，所有信号变量初始化赋值为2
  for(const auto& gate: gateinfo)
    CHECK(gate.m_val == 2);

  for(int i = 0; i < 3; i++)
    CHECK(gateinfo[i].m_watch_value.size() == 0);

  for(int i = 3; i < gateinfo.size(); i++)
    CHECK(gateinfo[i].m_watch_value.size() == 3);

  //监视值初始化检查  8 6 5
  gateinfo[3].m_watch_value[0] == 0;
  gateinfo[3].m_watch_value[1] == 1;
  gateinfo[3].m_watch_value[2] == 0;
  //监视值初始化检查  10 9 5
  gateinfo[4].m_watch_value[0] == 0;
  gateinfo[4].m_watch_value[1] == 0;
  gateinfo[4].m_watch_value[2] == 0;
  //监视值初始化检查  12 11 2
  gateinfo[5].m_watch_value[0] == 0;
  gateinfo[5].m_watch_value[1] == 0;
  gateinfo[5].m_watch_value[2] == 1;
  //监视值初始化检查  14 6 4
  gateinfo[6].m_watch_value[0] == 0;
  gateinfo[6].m_watch_value[1] == 1;
  gateinfo[6].m_watch_value[2] == 1;
  //监视值初始化检查  16 14 3
  gateinfo[7].m_watch_value[0] == 0;
  gateinfo[7].m_watch_value[1] == 1;
  gateinfo[7].m_watch_value[2] == 0;
  //监视值初始化检查  18 17 13
  gateinfo[8].m_watch_value[0] == 0;
  gateinfo[8].m_watch_value[1] == 0;
  gateinfo[8].m_watch_value[2] == 0;
  

}

TEST_CASE( "decision & bcp test", "[maj]" )
{
  solver solve = run_cirsat_maj().m_solver;
  std::vector<GateInfo> gateinfo = solve.get_m_val_info();
  std::vector<solver::DecInfor>& decinfo = solve.get_decision_target();
  CHECK( decinfo.size() == 3 );
  CHECK( decinfo[0].dec_line == 8 );
  CHECK( gateinfo[decinfo[0].dec_line].m_level == 0 );
  CHECK( gateinfo[decinfo[0].dec_line].m_val == 1 );
  //PO端含inverter
  CHECK( gateinfo[decinfo[0].dec_line].m_watch_value[0] == 1 );
  //决策线dec_line的赋值m_val等于其监视值，因此BCP无法推理除信息
  CHECK( decinfo[0].assigned_in_level.size() == 1 );
  CHECK( decinfo[0].assigned_in_level[0] == 8 );

  // std::cout<<*(decinfo[1].j_nodes.begin())<<std::endl;
  // std::cout<<"decision 2: "<<decinfo[1].dec_line<<std::endl;
  // std::cout<<"decision 3: "<<decinfo[2].dec_line<<std::endl;
  

}

// TEST_CASE( "bcp ", "[maj]" )
// {
//   assert(true);

// }

// TEST_CASE( "conflict ", "[maj]" )
// {
//   assert(true);

// }
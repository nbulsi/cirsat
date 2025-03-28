#include "aig.h"
#include <algorithm>
#include <time.h>

#define HAVE_NO_LEVEL 0xffffffff
struct solver_parameter
{
  uint32_t conflict_num = 100000;
};

enum class Stats{ 
  SAT,
  UNSAT,
  BCPSAT,
  BCPCON,
  UNKNOWN
};

struct GateInfo
{ 
  uint8_t                m_val = 2;                     //节点的output值，也称为变量的值
  uint32_t               m_level = HAVE_NO_LEVEL;       //所在的决策层
  uint32_t               m_active_value = 0;            //变量的活动值
  std::vector<GateId>    m_source;                      //source

  std::vector<uint32_t>  m_pins;                        //and: output input1 input2; 
  std::vector<uint8_t>   m_watch_value;                 //该节点的监视值
  std::vector<bool>      m_watch_points;                //监视指针
};

class solver
{
public:
  using jnodes = std::set<GateId>;
public:
  solver() = delete;
  solver(const AigGraph& graph, solver_parameter par = {});
  Stats run(int &ConfLimit, int &Verbose);
  void ShowResult(const AigGraph& graph, Stats, int&);


public:
struct DecInfor
  {
    uint32_t dec_line;
    std::vector<uint32_t> assigned_in_level;
    jnodes j_nodes;
  };

  //for test
  std::vector<GateInfo>& get_m_val_info(){return m_val_info;}
  std::vector<DecInfor>& get_decision_target(){return mdi;}

private:
  Stats BCP();
  Stats ConflictAnalysisAndBacktrack();
  Stats ForwardDirectImplication(const GateId& n, std::vector<uint32_t> &bcp_vec);
  Stats BackwardDirectImplication(const GateId& n, std::vector<uint32_t> &bcp_vec);
  uint8_t IndirectImplication(std::vector<uint32_t>& watch_list, const int& n, std::vector<uint32_t> &bcp_vec, const int idx);
  uint8_t LIndirectImplication(std::vector<uint32_t>& watch_list, const int& n, std::vector<uint32_t> &bcp_vec, const int idx); 
  uint32_t FindDecisionTarget();
  void UpdateLearntGate(const uint32_t trace_line); 
  void UpdateActive(); 
  void CancelAssignment(const uint32_t dec_level);
  void FindSecondDecisionLevel();

private:
  void AddNodeToJf(const GateId& n, const int DevLev);
  void DeleteNodeToJf(const GateId& n, const int DevLev);
  

private:
  const AigGraph& m_graph;
  uint32_t m_gates_num;

  solver_parameter m_par;
  std::vector<GateInfo> m_val_info;
  std::vector<DecInfor> mdi;

  std::array<std::vector<std::vector<uint32_t>>, 2> m_watch_list;                    //监视列表
  std::vector<std::array<uint32_t, 5>> mf_dimp;                                      //向前直接蕴含图
  std::vector<std::array<std::vector<std::pair<uint32_t, uint8_t>>, 2>> mb_dimp;     //向后直接蕴含图
  std::vector<GateId> m_conf_lines;

  uint32_t m_confict_nums = 0;
  uint32_t m_decision_nums = 0;
  uint32_t m_cur_level = 0;

  clock_t bcp_time = 0, solve_time = 0, conflict_time = 0, parser_time = 0;

  
};
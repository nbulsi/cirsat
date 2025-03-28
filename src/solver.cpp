#include "../include/solver.h"
#include "../include/aig.h"

solver::solver(const AigGraph& graph, solver_parameter par) : m_graph(graph), m_par(par)
{
  uint32_t inputs_num = m_graph.get_inputs().size();
  uint32_t outputs_num = m_graph.get_outputs().size();
  m_gates_num = m_graph.get_gates().size();
  m_val_info.resize(m_gates_num);
  mf_dimp.resize(m_gates_num);
  mb_dimp.resize(m_gates_num);
  m_watch_list[0].resize(m_gates_num);
  m_watch_list[1].resize(m_gates_num);
  //图节点是严格按照先放输入，再放其他节点的
  //确定每个input的活动值
  for(uint32_t i = 0; i < inputs_num; i++)
  {
    m_val_info[i].m_active_value = m_graph.get_gate(i).get_outputs().size(); //活动性：扇出
  }
  //确定每个gate的活动值、监视值和监视指针、并把其放入监视列表
  for(uint32_t i = inputs_num; i < m_gates_num; i++)
  {
    const Gate& gate = m_graph.get_gate(i);
    const auto inputs = m_graph.get_gate(i).get_inputs();
    uint32_t input1 = inputs[0] / 2;
    uint32_t input2 = inputs[1] / 2;

    m_val_info[i].m_active_value = m_graph.get_gate(i).get_outputs().size() + 2; //活动性：扇出加扇如
    //与门的监视值是：0、1、1，如果边上有反向器，则监视值取反，只有output节点的输出边可能会带反相器
    m_val_info[i].m_pins.resize(3);
    m_val_info[i].m_watch_value.resize(3);
    m_val_info[i].m_watch_points.resize(3, true);
    m_val_info[i].m_pins[1] = input1;
    m_val_info[i].m_watch_value[1] = 1 - gate.get_inputs()[0] % 2;
    m_val_info[i].m_pins[2] = input2;
    m_val_info[i].m_watch_value[2] = 1 - gate.get_inputs()[1] % 2;
    if(gate.is_output() && gate.get_outputs()[0] == HAVE_INV)
      m_val_info[i].m_watch_value[0] = 1;
    else  
      m_val_info[i].m_watch_value[0] = 0;
    m_val_info[i].m_pins[0] = i;
    //把output和第一个input作为监视指针，并把其放入监视列表
    m_val_info[i].m_watch_points[2] = false;

    m_watch_list[m_val_info[i].m_watch_value[0]][i].push_back(gate.get_id());
    m_watch_list[m_val_info[i].m_watch_value[1]][input1].push_back(gate.get_id());
    // std::cout<<"m_watch_list["<<int(m_val_info[i].m_watch_value[0])<<"]["<<i<<"]"<<" push_back: "<<gate.get_id()<<std::endl;
    // std::cout<<"m_watch_list["<<int(m_val_info[i].m_watch_value[1])<<"]["<<input1<<"]"<<" push_back: "<<gate.get_id()<<std::endl;
    //构建直接蕴含图
    //输入节点没有向前(向PI方向)蕴含
    //AND节点向前蕴含规则：输出为非监视值，可以蕴含出两个输入都为监视值
    //AND节点向后蕴含规则：输入有一个为非监视值，可以蕴含出输出为监视值
    uint8_t watch_val0 = m_val_info[i].m_watch_value[0];
    uint8_t watch_val1 = m_val_info[i].m_watch_value[1];
    uint8_t watch_val2 = m_val_info[i].m_watch_value[2];
    //构造向前直接蕴含
    mf_dimp[i][0] = 1 - watch_val0;
    mf_dimp[i][1] = input1;
    mf_dimp[i][2] = watch_val1;
    mf_dimp[i][3] = input2;
    mf_dimp[i][4] = watch_val2;
    //构造向后直接蕴含
    mb_dimp[input1][1 - watch_val1].emplace_back(i, watch_val0);
    mb_dimp[input2][1 - watch_val2].emplace_back(i, watch_val0);
  }
}

void solver::ShowResult(const AigGraph& graph, Stats run_result, int& Verbose) {
  if (run_result == Stats::SAT) {
    std::cout << "SATISFIABLE" << std::endl;
  } 
  else if (run_result == Stats::UNSAT){
    std::cout << "UNSATISFIABLE" << std::endl;
  }
  else{
    std::cout << "UNKNOWN" << std::endl;
  }
  std::cout<<"CPU time: "<<1.0 * (solve_time)/1000<< "  ms" << std :: endl;
  std::cout<<"decisions: "<<m_decision_nums<<std::endl;
  std::cout<<"conficts: "<<m_confict_nums<<std::endl;
  if(Verbose == 1){
    std::cout<<"============================[ Problem Statistics ]====================================="<<"\n";
    std::cout<<"|   "<<"BCP time: "<<1.0 * (bcp_time)/1000<<"  ms                                       "<<"\n";
    std::cout<<"|   "<<"ConflictAnalyze time: "<<1.0 * (conflict_time)/1000<<"  ms                      "<<"\n";
    std::cout<<"|   "<<"PIs: "<<m_graph.get_inputs().size()<<"                                         "<<"\n";
    std::cout<<"|   "<<"Gates: "<<m_graph.get_inputs().size()<<"                                       "<<"\n";
    std::cout<<"|   ";
    for(int i = 0 ; i < m_graph.get_inputs().size(); i++){
      if(i % 8 ==0 && i != 0) {
        std::cout<<"  "<<std::endl;
        std::cout<<"|   ";
      }
      std::cout<<"pi"<<m_graph.get_inputs()[i]<<" = "<<int(m_val_info[m_graph.get_inputs()[i]].m_val)<<"; ";
    }

    std::cout<<std::endl;
    std::cout<<"======================================================================================="<<std::endl;
    // for(int i = 0; i < m_val_info.size(); i++){
    //   std::cout<<i<<", m_val = "<<int(m_val_info[i].m_val)<<", m_level = "<<m_val_info[i].m_level<<", m_active_value = "<<m_val_info[i].m_active_value<<", m_source = ";
    //   for(auto temp1 : m_val_info[i].m_source){
    //     std::cout<<int(temp1)<<" ";
    //   }
    //   std::cout<<std::endl;
    // }
  }
}

Stats solver::run(int &ConfLimit, int &Verbose)
{
  clock_t begin, end, bcp_begin, bcp_end, conflict_begin, confict_end;
  begin = clock();
  m_par.conflict_num = ConfLimit;
  const GateId root = m_graph.get_outputs()[0];
  // m_val_info[root].m_val = 1 - m_val_info[root].m_watch_value[0];
  m_val_info[root].m_val = 1;
  // std::cout<<"PO num = "<<m_graph.get_outputs().size()<<", num = "<<root<<", m_val = "<<int(m_val_info[root].m_val)<<std::endl;
  m_val_info[root].m_level = 0;

  uint32_t cur_level = 0;
  mdi.emplace_back();
  mdi[0].dec_line = root;
  AddNodeToJf(root, 0);               //root
  mdi[0].assigned_in_level.push_back(root);
  
  Stats result;
  bcp_begin = clock();
  result = BCP();
  bcp_end = clock();
  bcp_time += (bcp_end - bcp_begin);
  if (result == Stats::BCPCON)
  {
    end = clock();
    solve_time = (end - begin);
    ShowResult(m_graph, Stats::UNSAT, Verbose);
    return Stats::UNSAT;
  }

  while(1)
  {
    uint32_t Dec = FindDecisionTarget();

    if(Dec == 0xffffffff)
    {
      end = clock();
      solve_time = (end - begin);
      ShowResult(m_graph, Stats::SAT, Verbose);
      return Stats::SAT;
    }
    while(1)
    {
      bcp_begin = clock();
      result = BCP();
      bcp_end = clock();
      bcp_time += (bcp_end - bcp_begin);
      if(result == Stats::BCPSAT)
      {
        break;
      }
      else
      {
        conflict_begin = clock();
        result = ConflictAnalysisAndBacktrack();
        confict_end = clock();
        conflict_time += (confict_end - conflict_begin);
        if(result == Stats::UNSAT)
        {
          end = clock();
          solve_time = (end - begin);
          ShowResult(m_graph, result, Verbose);
          return Stats::UNSAT;
        }
        if(result == Stats::UNKNOWN)
        {
          end = clock();
          solve_time = (end - begin);
          ShowResult(m_graph, result, Verbose);
          return Stats::UNKNOWN;
        }
      }
    }
  }
  return Stats::UNKNOWN;
}

Stats solver::BCP()
{
  m_decision_nums ++;
  std::vector<uint32_t>& bcp_vec = mdi.back().assigned_in_level;
  uint32_t dir = bcp_vec.size() - 1, indir = dir;
  Stats result;
  while(dir < bcp_vec.size() || indir < bcp_vec.size())
  {
    while (dir < bcp_vec.size()) 
    {
      const GateId g = bcp_vec[dir];
      result = ForwardDirectImplication(g, bcp_vec);
      if (result == Stats::BCPCON) //直接蕴含发现冲突，即bcp冲突
      {
        return Stats::BCPCON;
      }
      result = BackwardDirectImplication(g, bcp_vec);
      if (result == Stats::BCPCON) //直接蕴含发现冲突，即bcp冲突
      {
        return Stats::BCPCON;
      }
      dir ++;
    }
    while (indir < bcp_vec.size()) 
    {
      auto& watch_list = m_watch_list[m_val_info[bcp_vec[indir]].m_val][bcp_vec[indir]];
      // std::cout<<"watch_list["<<int(m_val_info[bcp_vec[indir]].m_val)<<"]["<<bcp_vec[indir]<<"]"<<std::endl;
      uint8_t res;
      for(int i = 0; i < watch_list.size();)
      {
        const GateId cur = watch_list[i];
        if(cur < m_gates_num)
          res = IndirectImplication(watch_list, i, bcp_vec, indir);
        else
          res = LIndirectImplication(watch_list, i, bcp_vec, indir);

        if(res == 0)            //0: 间接蕴含发生冲突，退出BCP函数
          return Stats::BCPCON;
        else if(res == 1)      //1：间接蕴含的gate是满足的，即有赋值为非监视值，不改变监视指针，不修改监视列表，此时i++
          i++;
        else if(res == 3)    //3：间接蕴含推不出东西但修改了监视指针，因为修改监视指针后监视列表会删除当前元素，所以i不变
          continue;
        else 
          break;
      }
      if(res == 2)  //2：间接蕴含推出东西，退出间接蕴含，做直接蕴含。此时间接蕴含指针indir不变，因为监视列表还没做完；//(也不是最优的，因为再次做间接蕴含的时候会重复做一些节点)
        break;
      else  
        indir++;
    }
  }
  return Stats::BCPSAT;
}

Stats solver::ForwardDirectImplication(const GateId& n, std::vector<uint32_t> &bcp_vec)
{
  //std::vector<std::array<uint32_t, 5>> mf_dimp;   
  //AND节点向前蕴含规则：输出为非监视值，可以蕴含出两个输入都为监视值
  //1:输出为监视值，蕴含不出任何东西
  if(m_val_info[n].m_val != mf_dimp[n][0] || n < m_graph.get_inputs().size())
    return Stats::BCPSAT;
  const std::array<uint32_t, 5u>& temp = mf_dimp[n];
  //2:输出为非监视值
  DeleteNodeToJf(n, m_cur_level);
  if(m_val_info[temp[1]].m_val == 2)
  {
    AddNodeToJf(temp[1], m_cur_level);
    m_val_info[temp[1]].m_val = temp[2];
    m_val_info[temp[1]].m_level = m_cur_level;
    m_val_info[temp[1]].m_source.push_back(n);
    bcp_vec.push_back(temp[1]);
  }
  else if(m_val_info[temp[1]].m_val == temp[2]) {}
  else
  {
    m_conf_lines = m_val_info[n].m_source;
    m_conf_lines.push_back(temp[1]);
    return Stats::BCPCON;
  }

  if(m_val_info[temp[3]].m_val == 2)
  {
    AddNodeToJf(temp[3], m_cur_level);
    m_val_info[temp[3]].m_val = temp[4];
    m_val_info[temp[3]].m_level = m_cur_level;
    m_val_info[temp[3]].m_source.push_back(n);
    bcp_vec.push_back(temp[3]);
  }
  else if(m_val_info[temp[3]].m_val == temp[4]) {}
  else
  {
    m_conf_lines = m_val_info[n].m_source;
    m_conf_lines.push_back(temp[3]);
    return Stats::BCPCON;
  }
  return Stats::BCPSAT;
}

Stats solver::BackwardDirectImplication(const GateId& n, std::vector<uint32_t> &bcp_vec)
{
  //std::vector<std::array<std::vector<std::pair<uint32_t, uint8_t>>, 2>> mb_dimp; 
  //AND节点向后蕴含规则：输入有一个为非监视值，可以蕴含出输出为监视值
  uint8_t this_val = m_val_info[n].m_val;
  const std::vector<std::pair<uint32_t, uint8_t>>& temp = mb_dimp[n][this_val];
  // std::cout<<" mb_dimp["<<n<<"]["<<int(this_val)<<"].size = "<< mb_dimp[n][this_val].size()<<std::endl;
  for(const auto& t : temp)
  {
    if(m_val_info[t.first].m_val == 2)
    {
      m_val_info[t.first].m_val = t.second;
      m_val_info[t.first].m_level = m_cur_level;
      m_val_info[t.first].m_source.push_back(n);
      bcp_vec.push_back(t.first);
      continue;
    }
    else if(m_val_info[t.first].m_val == t.second)
    {
      DeleteNodeToJf(t.first, m_cur_level);
    }
    else
    {
      m_conf_lines = m_val_info[n].m_source;
      m_conf_lines.push_back(t.first);
      return Stats::BCPCON;
    }
  }
  return Stats::BCPSAT;
}

uint8_t solver::IndirectImplication(std::vector<uint32_t>& watch_list, const int& cur, std::vector<uint32_t> &bcp_vec, const int indir)
{
  /*
  n
  1：有一个非监视值，退出，因为gate满足 
  2: conf: 所有值n都为监视值
  3：n - 1, x为非监视值, 推理出x赋值
  4：两个X，1个监视值, nothing to be deduced
  */
  const GateId cur_gate = watch_list[cur];
  GateInfo& cur_gate_info = m_val_info[cur_gate];

  const auto& lines = cur_gate_info.m_pins;
  const auto& wv = cur_gate_info.m_watch_value;

  std::vector<uint32_t> assign_info1;   //存储赋值为监视值的gate
  std::vector<std::pair<uint32_t, uint8_t>> assign_info2;   //存储未被赋值的gate
  for(int i = 0; i < 3; i++)
  {
    if(m_val_info[lines[i]].m_val == wv[i])
      assign_info1.push_back(lines[i]);
    else if(m_val_info[lines[i]].m_val == 2)
      assign_info2.emplace_back(lines[i], wv[i]);
    else 
      return 1;
  }
  //1: conf: 所有赋值都为监视值
  if(assign_info1.size() == 3)
  {
    m_conf_lines = lines;
    return 0;
  }
  //2：有一个x，其余都为监视值
  if(assign_info2.size() == 1)
  {
    const auto& temp = assign_info2[0];
    DeleteNodeToJf(cur_gate, m_cur_level);
    AddNodeToJf(temp.first, m_cur_level);
    m_val_info[temp.first].m_val = 1 - wv[temp.second];
    m_val_info[temp.first].m_level = m_cur_level;
    m_val_info[temp.first].m_source = assign_info1;
    bcp_vec.push_back(temp.first);
    return 2;
  }
  //3：有两个X，更改监视指针，必然只修改一个，并且是找到当前节点的监视指针;
  if(assign_info2.size() == 2)
  {
    uint32_t cur_point = bcp_vec[indir];
    watch_list.erase(watch_list.begin() + cur);
    //修改监视指针
    int idx = 0;
    for(int i = 0; i < 3; i++)
    {
      if(cur_gate_info.m_watch_points[i] && lines[i] == cur_point)
      {
        cur_gate_info.m_watch_points[i] = false;
        continue;
      }
      if(!cur_gate_info.m_watch_points[i])
      {
        cur_gate_info.m_watch_points[i] = true;
        idx = i;
      }
    }
    m_watch_list[wv[idx]][lines[idx]].push_back(cur_gate);
    // std::cout<<"m_watch_list["<<int(wv[idx])<<"]["<<lines[idx]<<"].push_back: "<<cur_gate<<std::endl;
    return 3;
  }
}

uint8_t solver::LIndirectImplication(std::vector<uint32_t>& watch_list, const int& cur, std::vector<uint32_t> &bcp_vec, const int indir)
{
  /*
  n
  1：有一个非监视值，退出 
  2: conf: 所有值n都为监视值
  3：n - 1, x为非监视值
  4：两个及以上X，其余全为监视值
  */
  const GateId cur_gate = watch_list[cur];
  GateInfo& cur_gate_info = m_val_info[cur_gate];

  const auto& lines = cur_gate_info.m_pins;
  const auto& wv = cur_gate_info.m_watch_value;
  auto& points = cur_gate_info.m_watch_points;
  std::array<std::vector<uint32_t>, 2u> assign_info;
  for(int i = 0; i < lines.size(); i++)
  {
    if(m_val_info[lines[i]].m_val == wv[i])
      assign_info[0].push_back(i);
    else if(m_val_info[lines[i]].m_val == 2)
      assign_info[1].push_back(i);
    else 
      return 1;
  }
  //1: conf: 所有赋值都为监视值
  if(assign_info[0].size() == lines.size())
  {
    m_conf_lines = lines;
    return 0;
  }
  //2：有一个x，其余都为监视值
  if(assign_info[1].size() == 1)
  {
    const auto& temp = assign_info[1][0];
    const uint32_t line = lines[temp];
    AddNodeToJf(line, m_cur_level);
    m_val_info[line].m_val = 1 - wv[temp];
    m_val_info[line].m_level = m_cur_level;
    for(const auto& t : assign_info[0])
      m_val_info[line].m_source.push_back(lines[t]);
    bcp_vec.push_back(line);
    return 2;
  }
  //3：有两个及两个以上X，更改监视指针
  if(assign_info[1].size() > 1)
  {
    uint32_t cur_point = bcp_vec[indir];
    //修改监视指针，将指向监视值的监视指针换为指向X
    watch_list.erase(watch_list.begin() + cur);
    int num = 0; //记录需要更改的监视指针的个数

    //需要更改的监视指针满足：1，是监视指针；2，被赋为监视值; 取消需要更改的监视指针，并删除其在监视列表中的节点索引
    for(const int& t : assign_info[0])
    { 
      if(points[t] == true && m_val_info[lines[t]].m_val == wv[t])
      {
        num ++ ;
        points[t] = false; 
        if(lines[t] == cur_point) //已经删除过了
          continue;
        auto& wl = m_watch_list[wv[t]][lines[t]];
        auto it = std::find(wl.begin(), wl.end(), cur_gate);
        wl.erase(it);
      }
    }
    //添加新的监视指针，并插入其在监视列表中的节点索引
    int i = 0;
    while(num)
    {
      for(; i < assign_info[1].size(); i++)
      {
        int temp = assign_info[1][i];
        if(points[temp] == false) //可以被候选为监视指针的线满足：1：之前不是监视指针，2：未被赋值
        { 
          num--;
          points[temp] == true;
          m_watch_list[wv[temp]][lines[temp]].push_back(cur_gate);
          break;
        }
      }
    }
    return 3;
  }
}

uint32_t solver::FindDecisionTarget()
{
  const jnodes& jf = mdi.back().j_nodes;
  std::unordered_map<uint32_t, std::vector<uint32_t>> count;
  //jnode的输出线赋值必然为监视值
  for(const auto& n : jf)
  {
    // std::cout << "j-node = " << n << " ";
    const GateInfo& temp = m_val_info[n];
    for(int i = 1; i < temp.m_pins.size(); i++)
    {
      if(m_val_info[temp.m_pins[i]].m_val == 2)
      {
        //只有将输入赋值为非监视值，才可以使得该节点被满足
        uint32_t t = temp.m_pins[i] * 2 + (1 - temp.m_watch_value[i]);
        if(count.find(t) == count.end()) //如果不存在，去构造
          count.emplace(t, std::vector<uint32_t>());
        count.at(t).push_back(n);
      }
    }
  }
  // std::cout << std::endl;
  uint32_t DecLine = 0xffffffff;
  uint32_t max_act = 0;
  for(const auto& t : count)
  {
    if(t.second.size() > max_act)
    {
      DecLine = t.first;
      max_act = t.second.size();
    }
  }

  if(DecLine == 0xffffffff)
    return DecLine;

  uint32_t line = DecLine / 2;
  uint8_t assign = DecLine % 2;
  // std::cout<<"DecLine = "<<line<<", assign = "<<int(assign)<<std::endl;
  m_cur_level = mdi.size();
  mdi.emplace_back();
  mdi.back().dec_line = line;
  mdi.back().j_nodes = mdi.at(mdi.size() - 2 ).j_nodes;
  AddNodeToJf(line, m_cur_level);
  mdi.back().assigned_in_level.push_back(line);
  m_val_info[line].m_val = assign;
  m_val_info[line].m_level = m_cur_level;

  return DecLine;
}

Stats solver::ConflictAnalysisAndBacktrack()
{
  // std::cout<<" enter ConflictAnalysisAndBacktrack"<<std::endl;
  m_confict_nums ++;
  if(m_confict_nums > m_par.conflict_num)
  {
    return Stats::UNKNOWN;
  }
  //冲突分析
  int this_level_count = 0;
  int line_decision_level;
  int trace_line = 0;       // line whose previous reason gate will next be used to resolve
  // std::cout<<"current decision level = "<<m_cur_level<<std::endl;
  do                        //do--while  for get a learnt gate
  {
    this_level_count = 0;
    for (int i = 0; i < m_conf_lines.size(); i++) 
    {
      // std::cout<<m_conf_lines[i]<<", level = "<<m_val_info[m_conf_lines[i]].m_level<<std::endl;
      line_decision_level = m_val_info[m_conf_lines[i]].m_level;
      if (line_decision_level == 0)     //line_decision_level == 0 && m_val_info[m_conf_lines[i]].m_source.empty()
      {
        m_conf_lines.erase(m_conf_lines.begin() + i);
        i--;
        // if learnt_gate size=0, UNSAT
        if (m_conf_lines.size() == 0) 
        {
          // std::cout<<"sover.cpp 512"<<std::endl;
          return Stats::UNSAT;
        }
        continue;
      }
      if (line_decision_level == m_cur_level) // a line at the same decision level been found
      {
        this_level_count++;
      }
      // a line at the same decision level but not a decision line
      if (line_decision_level == m_cur_level && m_val_info[m_conf_lines[i]].m_source.size() != 0) 
      {
        trace_line = m_conf_lines[i];
      }
    }
    // std::cout<<"------"<<std::endl;
    // only one line at the same decision level means we have a UIP
    if (this_level_count == 1) {
      break; // get learnt gate,which only have one line at decision level
    }

    if (m_val_info[trace_line].m_source.size() != 0) 
    {
      UpdateLearntGate(trace_line);
      if (m_conf_lines.size() == 0) 
      {
        // std::cout<<"sover.cpp 536"<<std::endl;
        return Stats::UNSAT;
      }
    }
  } while(true);
  
  // delete is_fixed_value line,and set learnt line polarity from learnt gate
  for (int i = 0; i < m_conf_lines.size();) 
  {
    if (m_val_info[m_conf_lines[i]].m_level == 0 ) 
    {
      m_conf_lines.erase(m_conf_lines.begin() + i);
    } 
    else
      i++;
  }

  //回溯
  if(m_conf_lines.size() == 0) 
  {
    // std::cout<<"sover.cpp 555"<<std::endl;
    return Stats::UNSAT;
  }
  if(m_conf_lines.size() == 1)
  {
    const uint32_t temp = m_conf_lines[0];
    uint8_t temp_val = 1 - m_val_info[temp].m_val;
    CancelAssignment(0);
    m_val_info[temp].m_level = 0;
    m_val_info[temp].m_val = temp_val;
    mdi[0].assigned_in_level.push_back(temp);
    AddNodeToJf(temp, 0);
    m_conf_lines.clear();  //hkm change
    m_cur_level = 0;
  }
  if(m_conf_lines.size() > 1)
  {
    //1：m_conf_lines,寻找第二大决策层
    std::vector<GateInfo>& ValInfo = m_val_info;
    std::sort(m_conf_lines.begin(), m_conf_lines.end(), 
           [&](const uint32_t n1, const uint32_t n2) -> bool
              {
                return ValInfo[n1].m_level > ValInfo[n2].m_level;
              });
    //2：Lgate开内存，更新一系列信息
    ValInfo.emplace_back();
    GateInfo& cur = ValInfo.back();
    cur.m_val = 1;
    cur.m_level = HAVE_NO_LEVEL;
    cur.m_pins = m_conf_lines;
    for(const auto& t : m_conf_lines)
      cur.m_watch_value.push_back(m_val_info[t].m_val);
    cur.m_watch_points.resize(m_conf_lines.size(), false);
    //第一、第二大决策层的线作为学习门的pointer
    if( m_conf_lines.size() == 2)
    {
      cur.m_watch_points[0] = true;   
      cur.m_watch_points[1] = true;       
    }
    else
    {
      cur.m_watch_points[1] = true;   
      cur.m_watch_points[2] = true;       
    }
    UpdateActive();
    const uint32_t temp = m_conf_lines[0];
    uint8_t temp_val = 1 - ValInfo[temp].m_val;
    //3：回溯到第二大决策层
    CancelAssignment(ValInfo.at(m_conf_lines[1]).m_level);
    m_cur_level = mdi.size() - 1;
    //4：将第一大决策层的反值添加到assigned_in_level中
    mdi.back().assigned_in_level.push_back(m_conf_lines[0]);
    ValInfo[temp].m_val = temp_val;
    ValInfo[temp].m_level = m_cur_level;
    for (int i = 1; i < m_conf_lines.size(); i++)
    {
      ValInfo[temp].m_source.push_back(m_conf_lines[i]);
    }
    AddNodeToJf(m_conf_lines[0], m_cur_level);
    //5：清理
    m_conf_lines.clear();
  }
  return Stats::BCPSAT;
}

void solver::AddNodeToJf(const GateId& n, const int DevLev)
{
  // std::cout<<"m_graph.get_inputs().size() = "<<m_graph.get_inputs().size()<<std::endl;
  //if(!n < m_graph.get_inputs().size())
  if(n >= m_graph.get_inputs().size())
  {
    mdi.at(DevLev).j_nodes.insert(n);
  }
}

void solver::DeleteNodeToJf(const GateId& n, const int DevLev)
{
  mdi.at(DevLev).j_nodes.erase(n);
}

void solver::UpdateLearntGate(const uint32_t trace_line)
{
  // find trace_line's source lines
  std::vector<GateId> trace_gate(m_val_info[trace_line].m_source);
  // concatenate the two
  m_conf_lines.insert(m_conf_lines.end(), trace_gate.begin(), trace_gate.end());
  for (int i = 0; i < m_conf_lines.size(); i++) {
    // remove the trace_line from the concatenated version learnt gate
    if (m_conf_lines[i] == trace_line) {
      m_conf_lines.erase(m_conf_lines.begin() + i);
      // i--;  //hkm  change
      break;
    }
  }
  // remove duplicates from the last concatenated version learnt gate
  std::sort(m_conf_lines.begin(), m_conf_lines.end());
  m_conf_lines.erase(std::unique(m_conf_lines.begin(), m_conf_lines.end()), m_conf_lines.end());
}

void solver::CancelAssignment(const uint32_t sec_max_dec_level)
{
  //恢复dec_level之前的所有信息
  for(uint32_t i = m_cur_level; i > sec_max_dec_level; i--)
  {
    for(const uint32_t& t : mdi[i].assigned_in_level)
    {
      GateInfo& temp = m_val_info[t];
      temp.m_val = 2;
      temp.m_level = HAVE_NO_LEVEL;
      temp.m_source.clear();
    }
  }
  mdi.erase(mdi.begin() + sec_max_dec_level + 1, mdi.end());
  m_cur_level = sec_max_dec_level;
}

void solver::UpdateActive()
{
  for(const auto& t : m_conf_lines)
    m_val_info[t].m_active_value ++;
}



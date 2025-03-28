#include <iostream>
#include <string>
#include <vector>

#ifndef MYFUNCTION_H
#define MYFUNCTION_H

static std::vector<std::string> m_split(const std::string& input, const std::string& pred)
{
  std::vector<std::string> result;
  std::string temp{""};
  unsigned count1 = input.size();
  unsigned count2 = pred.size();
  unsigned j;
  for (size_t i = 0; i < count1; i++)
  {
    for(j = 0; j < count2; j++)
    {
      if(input[i] == pred[j])
      {
        break;
      }
    }
    //if input[i] != pred中的任何一个 该字符加到temp上
    if(j == count2)
      temp += input[i];
    else
    {
      if(!temp.empty())
      {
        result.push_back(temp);
        temp.clear();
      }
    }
  }
  if(!temp.empty())
  {
    result.push_back(temp);
    temp.clear();
  }
  return result;
}

static void delete_space(std::string& str)
{
  for(int i = str.size(); i > -1; i--)
  {
    if(str[i] == ' ' || str[i] == '\t')
    {
      str.erase(str.begin() + i);
    }
  }
}

static void seg_fault(const std::string& name, int size, int idx)
{
  std::cout << name << "  " << size << " : " << idx << std::endl; 
}


#endif
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>

#include <google/gflags.h>

#include "utils.hpp"
#include "balltree.hpp"
#include "search.hpp"

std::vector<std::vector<double> > get_item_factor(const std::string & fname) {
  std::vector<std::vector<double> > factor_vec_lst;
  std::ifstream f(fname);
  std::string line_buf;
  if(!f) { throw std::runtime_error("roraima error in get_item_factor: loading failed."); }
  while(std::getline(f, line_buf)) {
    auto tmp = roraima::str_split(line_buf, ":");
    auto tmp2 = roraima::str_split(tmp[1], "|");
    std::vector<double> temp;
    for(auto & fac : tmp2) {
      temp.push_back(std::stod(fac));
    }
    factor_vec_lst.push_back(std::move(temp));
  }
  return factor_vec_lst;
}

std::vector<double> get_usr_factor(const std::string & fname, const int offset = 0, char sep1 = ':', char sep2 = '|') {
  std::vector<double> factor_vec;
  std::ifstream f(fname);
  std::string line_buf;
  int cnt = 0;
  while(std::getline(f, line_buf) and cnt < offset) { cnt += 1; }
  auto tmp = roraima::str_split(line_buf, sep1);
  auto tmp2 = roraima::str_split(tmp[1], sep2);
  factor_vec.push_back(1.);
  for(int i = 1; i < (int)tmp2.size(); ++i) {
    factor_vec.push_back(std::stod(tmp2[i]));
  }
  return factor_vec;
}

std::vector<double> get_usr_factor(const std::string & fname, const std::string & usr_id, char sep1 = ':', char sep2 = '|') {
  std::vector<double> factor_vec;
  std::ifstream f(fname);
  std::string line_buf;
  while(std::getline(f, line_buf)) { 
    auto tmp = roraima::str_split(line_buf, sep1);
    if(tmp[0] == usr_id) {
      auto tmp2 = roraima::str_split(tmp[1], sep2);
      factor_vec.push_back(1.);
      for(int i = 1; i < (int)tmp2.size(); ++i) {
        factor_vec.push_back(std::stod(tmp2[i]));
      }
      break;
    }
  }
  return factor_vec;
}

DEFINE_string(item_factor_file, 
	"/home/xunzhang/xunzhang/Proj/parasol/tests/serial_mf/item_factor.csv", 
	"Item factor file generated by matrix factorization.\n\
	fmt: user_id:user_bias|fac_val_1|fac_val_2|...|fac_val_k.\n\
	':' and '|' can be replaced by sep1, sep2\n");
DEFINE_string(usr_factor_file, 
	"/home/xunzhang/xunzhang/Proj/parasol/tests/serial_mf/usr_factor.csv", 
	"User factor file generated by matrix factorization.\n\
	fmt: item_id:item_bias|fac_val_1|fac_val_2|...|fac_val_k.\n\
	':' and '|' can be replaced by sep1, sep2\n");
DEFINE_int64(topk, 10, "top k maximum rating(default kernel: inner product).\n");

int main(int argc, char *argv[]) 
{
  google::SetUsageMessage("[options]\n\
  			--item_factor_file\titem factor\n\
			--usr_factor_file\tuser factor\n\
			--topk\n");
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::vector<std::size_t> answer;
  auto item_factor_lst = get_item_factor(FLAGS_item_factor_file);
  roraima::balltree<double, roraima::eculid_dist> stree(item_factor_lst);
  stree.build();
  
  std::cout << "usr_id: " << std::endl;
  std::string s;
  while(std::cin >> s) { 
    //auto user_factor = get_usr_factor("/home/xunzhang/xunzhang/Proj/parasol/tests/serial_mf/usr_factor.csv", 1);
    auto user_factor = get_usr_factor(FLAGS_usr_factor_file, s);
    roraima::query q(user_factor, FLAGS_topk);
    int cnt = roraima::search(q, stree, answer);
    std::cout << "span cnt: " << cnt << " out of " << item_factor_lst.size() << std::endl;
    for(auto & indx : answer)
      std::cout << indx << std::endl;
    std::cout << "usr_id: " << std::endl;
  }
  return 0;
}

#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

#include <google/gflags.h>

#include "utils.hpp"
#include "balltree.hpp"
#include "search.hpp"
#include "cache.hpp"

std::vector<std::vector<double> > 
get_item_factor(const std::string & fname) {
  std::vector<std::vector<double> > factor_vec_lst;
  std::ifstream f(fname); std::string line_buf;
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
  f.close();
  return factor_vec_lst;
}

void get_item_indices_and_top_ibias_ids(const std::string & fname,
			std::unordered_map<long, long> & indices_dct,
			std::unordered_map<long, long> & reverse_indices_dct,
			std::vector<long> & top_ibias_ids, int topk) {
  std::ifstream f(fname); std::string line_buf;
  vector<std::pair<long, double> > d;
  if(!f) { throw std::runtime_error("roraima error in get_item_indices_and_bias: loading failed,"); }
  long cnt = 0;
  while(std::getline(f, line_buf)) {
    auto tmp = roraima::str_split(line_buf, ":");
    auto tmp2 = roraima::str_split(tmp[1], "|");
    indices_dct[cnt] = std::stol(tmp[0]);
    reverse_indices_dct[std::stol(tmp[0])] = cnt;
    d.push_back(std::pair<long, double>(std::stol(tmp[0]), std::stod(tmp2[0])));
    cnt += 1;
  }
  std::sort(d.begin(), d.end(),
    [] (std::pair<long, double> a,
    std::pair<long, double> b) {
    return a.second > b.second;
  });
  for(auto & kv : d) {
    top_ibias_ids.push_back(kv.first);
    if((int)top_ibias_ids.size() == topk) break;
  }
  f.close();
}

std::unordered_map<std::string, long> 
get_meta_dct(const std::string & fname) {
  std::unordered_map<std::string, long> meta_dct;
  std::ifstream f(fname); std::string line_buf;
  if(!f) { throw std::runtime_error("roraima error in get_usr_factor_lst: loading failed."); }
  long offset = 0;
  while(std::getline(f, line_buf)) {
    auto tmp = roraima::str_split(line_buf, ":");
    meta_dct[tmp[0]] = offset;
    offset = f.tellg();
  }
  f.close();
  return meta_dct;
}

std::vector<long> 
get_artist_track_ids(const std::string & fname, 
		const long offset = 0, 
		char sep1 = ':', 
		char sep2 = '|') {
  std::vector<long> ids;
  std::ifstream f(fname); std::string line_buf;
  f.seekg(offset);
  std::getline(f, line_buf);
  auto tmp = roraima::str_split(line_buf, sep1);
  auto tmp2 = roraima::str_split(tmp[1], sep2);
  for(auto & v: tmp2) {
    ids.push_back(std::stol(v));
  }
  return ids;
}

std::unordered_map<long, char> 
get_black_trackids(const std::string & fname, 
                const std::string & fname2,
		std::unordered_map<long, long> & d, 
		std::unordered_map<std::string, long> & meta,
		const long offset = 0, 
		char sep1 = ':', 
		char sep2 = '|') {
  std::unordered_map<long, char> ids;
  std::ifstream f(fname); std::string line_buf;
  f.seekg(offset);
  std::getline(f, line_buf);
  auto tmp = roraima::str_split(line_buf, sep1);
  auto tmp2 = roraima::str_split(tmp[1], sep2);
  for(int i = 1; i < (int)tmp2.size(); ++i) {
    if(roraima::startswith(tmp2[i], "a")) {
      auto str_id = roraima::str_split(tmp2[i], "a")[0];
      auto related_ids = get_artist_track_ids(fname2, meta[str_id]);
      for(auto & id : related_ids) {
        ids[d[id]] = '0';
      }
    } else {
      ids[d[std::stol(tmp2[i])]] = '0';
    }
  }
  f.close();
  return ids;
}

std::vector<double> get_usr_factor(const std::string & fname, 
				const long offset = 0, 
				char sep1 = ':', 
				char sep2 = '|') {
  std::vector<double> factor_vec;
  std::ifstream f(fname);
  std::string line_buf;
  f.seekg(offset);
  std::getline(f, line_buf);
  auto tmp = roraima::str_split(line_buf, sep1);
  auto tmp2 = roraima::str_split(tmp[1], sep2);
  factor_vec.push_back(1.);
  for(int i = 1; i < (int)tmp2.size(); ++i) {
    factor_vec.push_back(std::stod(tmp2[i]));
  }
  return factor_vec;
}

std::vector<double> get_usr_factor(const std::string & fname, 
				const std::string & usr_id, 
				char sep1 = ':', 
				char sep2 = '|') {
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
	"item_factor.csv", 
	"Item factor file generated by matrix factorization.\n\
	fmt: user_id:user_bias|fac_val_1|fac_val_2|...|fac_val_k.\n\
	':' and '|' can be replaced by sep1, sep2\n");

DEFINE_string(usr_factor_file, 
	"usr_factor.csv", 
	"User factor file generated by matrix factorization.\n\
	fmt: item_id:item_bias|fac_val_1|fac_val_2|...|fac_val_k.\n\
	':' and '|' can be replaced by sep1, sep2\n");

DEFINE_string(usr_blacklst_file, 
	"usr_blacklst_dict.csv",
	"fmt: user_id:item_id1|item_id2|'a'artist_id.\n\
	':' and '|' can be replaced by sep1, sep2\n");

DEFINE_string(artist_track_file, 
	"artist_track_dict.csv",
	"fmt: artist_id:item_id1|item_id2|...|item_idk.\n\
	':' and '|' can be replaced by sep1, sep2\n");

static bool ValidateMethod(const char* flagname, const std::string & method) {
  if(method == "linear" || method == "tree") return true;
  return false;
}

DEFINE_string(method, "tree", "search method: linear | tree\n");
static const bool method_dummy = google::RegisterFlagValidator(&FLAGS_method, &ValidateMethod);

DEFINE_int64(topk, 50, "top k maximum rating(default kernel: inner product).\n");

DEFINE_int64(cache_sz, 1000, "cache size.\n");

int main(int argc, char *argv[]) 
{
  google::SetUsageMessage("[options]\n\
  			--item_factor_file\titem factor\n\
			--usr_factor_file\tuser factor\n\
			--usr_blacklst_file\tuser black list\n\
			--artist_track_file\tartist track mapping file \n\
			--method\tsearch method\n\
			--topk\n\
			--cache_sz\n");
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::vector<long> answer;
  std::vector<long> top_ibias_ids; 
  std::unordered_map<long, long> item_indices_dct, reverse_item_indices_dct;
  roraima::lru_cache<std::string, std::vector<long> > cache(FLAGS_cache_sz);

  // load item factor and construct balltree
  auto item_factor_lst = get_item_factor(FLAGS_item_factor_file);
  roraima::balltree<double, roraima::eculid_dist> stree(item_factor_lst);
  if(FLAGS_method == "tree") {
    stree.build();
  }
  /* init:
   *   item_indices_dct - {indx : iid}
   *   reverse_item_indices_dct - {iid : indx}
   *   top_ibias_ids - {iid}
   */
  get_item_indices_and_top_ibias_ids(FLAGS_item_factor_file, 
  				item_indices_dct, 
				reverse_item_indices_dct, 
				top_ibias_ids, 
				FLAGS_topk);

  auto usr_factor_meta = get_meta_dct(FLAGS_usr_factor_file);
  auto blacklst_meta = get_meta_dct(FLAGS_usr_blacklst_file);
  auto artist_meta = get_meta_dct(FLAGS_artist_track_file);

  std::string s;
  std::vector<double> user_factor;
  std::unordered_map<long, char> black_ids; // {indx : '0'}
  // main loop 
  while(std::cin >> s) { 
    
    if(usr_factor_meta.find(s) != usr_factor_meta.end()) {
      user_factor = get_usr_factor(FLAGS_usr_factor_file, 
      				usr_factor_meta[s]);
    } else {
      // output
      for(auto & indx : top_ibias_ids) {
        std::cout << indx << std::endl;
      }
      continue; 
    }

    // load trackids in blacklst
    if(blacklst_meta.find(s) != blacklst_meta.end()) {
      black_ids = get_black_trackids(FLAGS_usr_blacklst_file, 
      				FLAGS_artist_track_file,
      				reverse_item_indices_dct, 
				artist_meta,
				blacklst_meta[s]);
    } else {
      // black_ids is empty
    }
    
    // init query
    roraima::query q(black_ids, 
    		user_factor, 
		FLAGS_topk);
    
    answer = cache.Get(s);
    if(!answer.size()) {
      if(FLAGS_method == "tree") {
        roraima::search(q, stree, answer);
      } else if(FLAGS_method == "linear"){
        // linear
        roraima::search(q, item_factor_lst, answer); 
      }
    } else {}
    cache.Put(s, answer);

    // output answer
    for(auto & indx : answer) {
      std::cout << item_indices_dct[indx] << std::endl;
    }
  }
  return 0;
}

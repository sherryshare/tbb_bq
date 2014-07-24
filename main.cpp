#include <iostream>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include "bq.h"
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <functional>
// #include <ff.h>

// #include <tbb/mutex.h>
#include <tbb/task_scheduler_init.h>
// #include <tbb/task_group.h>



namespace bpo = boost::program_options;


int main(int argc, char **argv) {
  
  int iThreadNum = 4;
  int iMsgNum = 100000;
  bpo::options_description boptions("Buffering Queue Benchmark for Function Flow");
  boptions.add_options()
  ("help", "Print help message.")
  ("serial", "Run serial version.")
  ("threads", bpo::value<int>(), boost::str(boost::format("Specify threads number in thread pool, default is %1%") % iThreadNum).c_str())
  ("std-lock", "Using standard lock.")
  ("tbb-lock", "Using lock provided by TBB.")
  ("msg-num", bpo::value<int>(), boost::str(boost::format("Specify message number to use, default is %1%") % iMsgNum).c_str());
  
  bpo::variables_map mvMap;
  bpo::store(bpo::parse_command_line(argc, argv, boptions), mvMap);
  bpo::notify(mvMap);
  
  //Check parameters!
  if(mvMap.count("help"))
  {
    std::cout<<boptions<<std::endl;
    return 0;
  }
  int action = 0;
  if(mvMap.count("serial"))
    action ++;
  if(mvMap.count("std-lock"))
    action ++;
  if(mvMap.count("tbb-lock"))
    action ++;
  
  if(action != 1)
  {
    std::cout<<"No action or too many actions!"<<std::endl;
    std::cout<<boptions<<std::endl;
    return 0;
  }
  if(mvMap.count("msg-num"))
  {
    iMsgNum = mvMap["msg-num"].as<int>();
  }

  if(mvMap.count("threads"))
  {
    if( mvMap.count("serial"))
    {
      std::cout<<"Cannot specify \"threads\" while using \"serial\""<<std::endl;
      std::cout<<boptions<<std::endl;
      return 0;
    }
    int t = mvMap["threads"].as<int>();
    if( t<= 0)
    {
      std::cout<<boptions<<std::endl;
      return 0;
    }
    iThreadNum = t;
  }
  
  //done checking parameters
  
  //setting thread number
//   ff::rt::set_hardware_concurrency(iThreadNum);//Set concurrency
//   ff::para<> a;
//   a([]() {});
//   ff::ff_wait(a);
  tbb::task_scheduler_init init(iThreadNum);//Set concurrency
  
  std::string output_file = "time.json";
  boost::property_tree::ptree pt;
  pt.put("time-unit", "us");
//   ff::scope_guard __t([](){}, [&pt](){boost::property_tree::write_json("time.json", pt);});
  
  std::function<void (const std::function<void ()> &)> time = [&pt](const std::function<void () > & f){
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    f();
    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    pt.put("time", elapsed);
    std::cout<<"time: "<<elapsed <<" us"<<std::endl;
    boost::property_tree::write_json("time.json", pt);
  };
  
  
  if (mvMap.count("serial"))
  {
    time([iMsgNum](){serial(iMsgNum);});
    return 0;
  }
  if (mvMap.count("std-lock"))
  {
    time([iMsgNum, iThreadNum](){parallel(iMsgNum, iThreadNum, false);});
    return 0;
  }
  if(mvMap.count("tbb-lock"))
  {
    time([iMsgNum, iThreadNum](){parallel(iMsgNum, iThreadNum, true);});
    return 0;
  }
    return 0;
}

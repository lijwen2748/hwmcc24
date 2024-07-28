/*
    Copyright (C) 2018, Jianwen Li (lijwen2748@gmail.com), Iowa State University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "carChecker.h"
#include "bmcChecker.h"
#include "statistics.h"
#include "data_structure.h"
#include "model.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <assert.h>
using namespace std;
using namespace car;

namespace car {
  Statistics stats;
  ofstream dot_file;
  ofstream dive_file;
  Checker *chk;
  bool verbose = false;
  bool verbose_ = false;
  Model* model;
  const Model* State::model_;
  const aiger* State::aig_;
}

void signal_handler (int sig_num)
{
  std::cout<<"Time out"<<endl;
  if(dot_file.is_open())
  {
    if(chk)
    {
      chk->draw_graph();
    }
  }
  dot_file.close();
  if(dive_file.is_open())
  {
    if(chk)
    {
      chk->dive_draw_tail();
    }
    dive_file.close();
  }
   chk->print_U_shape();
	stats.count_total_time_end ();
	stats.print ();
	exit (0);
}

void print_usage () 
{
  printf ("Usage: simplecar <-f|-b|-p|-e|-v|-h> <aiger file> <output directory>\n");
  printf ("       -f          forward checking (Default = backward checking)\n");
  printf ("       -b          backward checking \n");
  printf ("       -p          enable propagation (Default = off)\n");
  printf ("       -e          print witness (Default = off)\n");
  printf ("       -v          print verbose information (Default = off)\n");
  printf ("       -h          print help information\n");
  exit (1);
}

string get_file_name (string& s)
{
    size_t start_pos = s.find_last_of ("/");
    if (start_pos == string::npos)
       start_pos = 0;
    else
        start_pos += 1;
         
    
    string tmp_res = s.substr (start_pos);
    
    string res = "";
    //remove .aig

    size_t end_pos = tmp_res.find (".aig");
    assert (end_pos != string::npos);
    
    for (int i = 0; i < end_pos; i ++)
        res += tmp_res.at (i);
        
    return res;
    
}

void check_aiger (int argc, char** argv)
{
   stats.count_before_car_start();
   bool forward = false;
   bool evidence = false;
   bool propagate = false;
   bool enable_bi_check = false;
   bool enable_draw = false;
   bool enable_dive = false;
   bool bmc = false;
   
   string input;
   string output_dir;
   bool input_set = false;
   bool output_dir_set = false;
   for (int i = 1; i < argc; i ++)
   {
   		if (strcmp (argv[i], "-f") == 0)
   			forward = true;
   		else if (strcmp (argv[i], "-b") == 0)
   			forward = false;
   		else if (strcmp (argv[i], "-p") == 0)
   			propagate = true;
   		else if (strcmp (argv[i], "-v") == 0)
   		{
      	verbose = true;   // used outside checker
        verbose_ = true;  // used inside checker
      }
   		else if (strcmp (argv[i], "-e") == 0)
   			evidence = true;
   		else if (strcmp (argv[i], "-h") == 0)
   			print_usage ();
      else if(strcmp (argv[i], "--bi") == 0)
        enable_bi_check = true;
      else if(strcmp (argv[i], "--draw") == 0)
      {
        evidence = true;
        enable_draw = true;
      }
      else if(strcmp (argv[i], "--dive") == 0)
      {
        enable_dive = true;
      }
      else if(strcmp (argv[i], "--bmc") == 0)
      {
        bmc = true;
      }
   		else if (!input_set)
   		{
   			input = string (argv[i]);
   			input_set = true;
   		}
   		else if (!output_dir_set)
   		{
   			output_dir = string (argv[i]);
   			output_dir_set = true;
   		}
   		else
   			print_usage ();
   }
   if (!input_set || !output_dir_set)
   		print_usage ();

   if (output_dir.at(output_dir.size() - 1) != '/')
      output_dir += "/";
   std::string filename = get_file_name(input);

   std::string stdout_filename = output_dir + filename + ".log";
   std::string stderr_filename = output_dir + filename + ".err";
   std::string res_file_name = output_dir + filename + ".res";
   std::string trial_name = output_dir + filename + ".trail";
   std::string dot_name = output_dir + filename + ".dot";
   std::string dive_name = output_dir + filename + ".dive.dot";
   if (!verbose)
      auto fs = freopen(stdout_filename.c_str(), "w", stdout);
   ofstream res_file;
   res_file.open(res_file_name.c_str());
   ofstream trail_file;
#ifdef TRAIL
   trail_file.open(trial_name.c_str());
#endif

   if (enable_draw)
      dot_file.open(dot_name.c_str());
   if (enable_dive)
      dive_file.open(dive_name.c_str());

   stats.count_total_time_start();
   // get aiger object
   aiger *aig = aiger_init();
   aiger_open_and_read_from_file(aig, input.c_str());
   const char *err = aiger_error(aig);
   if (err)
   {
      printf("read aiger file error!\n");
      // throw InputError(err);
      exit(0);
   }
   if (!aiger_is_reencoded(aig))
      aiger_reencode(aig);

   stats.count_model_construct_time_start();
   Model *model = new Model(aig);
   // FIXME: collect all these static members. unify them.
   car::model = model;
   State::model_ = model;
   State::aig_ = aig;
   stats.count_model_construct_time_end();

   if (verbose)
      model->print();

   State::set_num_inputs_and_latches(model->num_inputs(), model->num_latches());

   // assume that there is only one output needs to be checked in each aiger model,
   // which is consistent with the HWMCC format
   assert(model->num_outputs() == 1);

   if (bmc)
   {
      auto bchker = new bmc::BMCChecker(model);
      bchker->check();
      bchker->printEvidence(res_file);
      return;
   }
   std::set<car::Checker*> to_clean;

   stats.global_start = high_resolution_clock::now();
   #ifdef PP_LIMIT_TIME
   // construct the checker
   int cnt = 0;
   chk = new Checker(cnt, model, res_file, trail_file, dot_file, dive_file, enable_dive, forward, evidence, 0, enable_bi_check);
   to_clean.insert(chk);

   bool res = chk->check();
   while(chk->ppstoped)
   {
      cout<<"restart at : "<<duration_cast<milliseconds>(high_resolution_clock::now() - stats.global_start).count()/1000 <<endl;
      ++cnt;
      chk = new Checker(cnt, model, res_file, trail_file, dot_file, dive_file, enable_dive, forward, evidence, 0, enable_bi_check);
      to_clean.insert(chk);
      cout<<"next checker ready at : "<<duration_cast<milliseconds>(high_resolution_clock::now() - stats.global_start).count()/1000 <<endl;
      cout<<"next strategy is : inter = " <<cnt<<endl;
      res = chk->check();
   }

   for(auto chker : to_clean) 
      if(chker)
         delete chker;
   #else
   chk = new Checker(model, res_file, trail_file, dot_file, dive_file, enable_dive, forward, evidence, 0, enable_bi_check);
   chk->check();
   delete chk;
   #endif

   // cleaning work
   aiger_reset(aig);
   delete model;
   res_file.close();
#ifdef TRAIL
   trail_file.close();
#endif
   if (enable_draw)
      dot_file.close();
   if (enable_dive)
      dive_file.close();
   stats.count_total_time_end();
   stats.print();
   return;
}

int main(int argc, char **argv)
{
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   check_aiger(argc, argv);

   return 0;
}

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



#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdlib.h>
#include <iostream>
#include <chrono>
using namespace std::chrono;
using clock_high = time_point<high_resolution_clock>;

namespace car {

class Statistics 
{
    public:
        Statistics () 
        {
            num_SAT_calls_ = 0;
            num_main_solver_SAT_calls_ = 0;
            num_inv_solver_SAT_calls_ = 0;
            num_start_solver_SAT_calls_ = 0;
            time_SAT_calls_ = 0.0;
            time_total_ = 0.0;
            time_main_solver_SAT_calls_ = 0.0;
            time_inv_solver_SAT_calls_ = 0.0;
            time_start_solver_SAT_calls_ = 0.0;
            time_model_construct_ = 0.0;
            num_reduce_uc_SAT_calls_ = 0;
            time_reduce_uc_SAT_calls_ = 0.0;
            compress_reduce_uc_ratio_ = 0.0;
            orig_uc_size_ = 0;
            reduce_uc_size_ = 0;
            num_clause_contain_ = 0;
        	num_state_contain_ = 0;
        	time_clause_contain_ = 0.0;
        	time_state_contain_ = 0.0;
            time_converting_lits = 0.0;
            
            time_before_car = 0.0;
            time_subgoal_ = 0.0;
            time_maingoal_ = 0.0;
            time_after_car = 0.0;
            time_cleaning = 0.0;
            nInAss = 0;
            // different sections of it.
            nInAss0_succeed = 0;
            nInAss1_succeed = 0;
            nInAss2_succeed = 0;
            converge_total = 0;
            converge_implied = 0;


        }
        ~Statistics () {}
        void print () 
        {
            std::cout << "Time of model construct: " << time_model_construct_ << std::endl;
            std::cout << "Num of total SAT Calls: " << num_SAT_calls_ << std::endl;
            std::cout << "      Num of main solver SAT Calls: " << num_main_solver_SAT_calls_ << std::endl;
            std::cout << "      Num of inv solver SAT Calls: " << num_inv_solver_SAT_calls_ << std::endl;
            std::cout << "      Num of start solver SAT Calls: " << num_start_solver_SAT_calls_ << std::endl;
            std::cout << "      Num of reduce uc SAT Calls: " << num_reduce_uc_SAT_calls_ << std::endl;
            // std::cout << "      Num of detect dead state SAT Calls: " << num_detect_dead_state_SAT_calls_ << std::endl;
            std::cout << "Time of total SAT Calls: " << time_SAT_calls_ << std::endl;
            std::cout << "      Time of main solver SAT Calls: " << time_main_solver_SAT_calls_ << std::endl;
            std::cout << "      Time of inv solver SAT Calls: " << time_inv_solver_SAT_calls_ << std::endl;
            std::cout << "      Time of start solver SAT Calls: " << time_start_solver_SAT_calls_ << std::endl;
            std::cout << "      Time of reduce uc SAT Calls: " << time_reduce_uc_SAT_calls_ << std::endl;
            // std::cout << "      Time of detect dead state SAT Calls: " << time_detect_dead_state_SAT_calls_ << std::endl;
            std::cout << "Num of detect dead state success: " << num_detect_dead_state_success_ << std::endl;
            std::cout << "Num of clause contain: " << num_clause_contain_ << std::endl;
            std::cout << "Time of clause contain: " << time_clause_contain_ << std::endl;
            std::cout << "Num of state contain: " << num_state_contain_ << std::endl;
            std::cout << "Time of state contain: " << time_state_contain_ << std::endl;
            std::cout << "Sum of original uc: " << orig_uc_size_ << std::endl;
            std::cout << "Sum of reduce uc: " << reduce_uc_size_ << std::endl;
            std::cout << "Reduce uc ratio: " << 1-(reduce_uc_size_/double (orig_uc_size_)) << std::endl;
            std::cout << "Time of converting lits: " << time_converting_lits <<std::endl;
            std::cout << "Time before CAR: " << time_before_car <<std::endl;
            std::cout << "Time for CAR: " << time_for_car <<std::endl;
            
            std::cout << "      Time for subgoal: " << time_subgoal_ <<std::endl;
            std::cout << "      Time for maingoal: " << time_maingoal_ <<std::endl;
            std::cout << "Time after CAR: " << time_after_car <<std::endl;
            std::cout << "      Time of cleaning: " << time_cleaning << std::endl;  
            std::cout << "Time for calculate next assumption: " << conv_calc_acc / 1000.0 <<std::endl;
            std::cout << "Time for calculate next UC: " << conv_second_solve_acc / 1000.0 << std::endl;  


            std::cout << "Total Time: " << time_total_ << std::endl;

            // std::cout << "uc fall in assumption precedence 0 :"<<nInAss0_succeed <<" / "<<nInAss<<std::endl;
            // std::cout << "uc fall in assumption precedence 1 :"<<nInAss1_succeed <<" / "<<nInAss<<std::endl;
            // std::cout << "uc fall in assumption precedence 2 :"<<nInAss2_succeed <<" / "<<nInAss<<std::endl;

            std::cout<<"next uc fall in previous: "<<converge_implied<<std::endl;
            std::cout<<"next uc total: "<<converge_total<<std::endl;
        }
        inline void count_SAT_time_start ()
        {
            begin_ = high_resolution_clock::now();
        }
        inline void count_SAT_time_end ()
        {
            end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        num_SAT_calls_ += 1;
	        time_SAT_calls_ += duration;
        }
        inline void count_main_solver_SAT_time_start ()
        {
            begin_ = high_resolution_clock::now();
        }
        inline void count_main_solver_SAT_time_end ()
        {
            end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        time_main_solver_SAT_calls_ += duration;
	        time_SAT_calls_ += duration;
	        num_main_solver_SAT_calls_ += 1;
	        num_SAT_calls_ += 1;
        }
        inline void count_inv_solver_SAT_time_start ()
        {
            begin_ = high_resolution_clock::now();
        }
        inline void count_inv_solver_SAT_time_end ()
        {
            end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        time_inv_solver_SAT_calls_ += duration;
	        time_SAT_calls_ += duration;
	        num_inv_solver_SAT_calls_ += 1;
	        num_SAT_calls_ += 1;
        }
        inline void count_start_solver_SAT_time_start ()
        {
            begin_ = high_resolution_clock::now();
        }
        inline void count_start_solver_SAT_time_end ()
        {
            end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        time_start_solver_SAT_calls_ += duration;
	        time_SAT_calls_ += duration;
	        num_start_solver_SAT_calls_ += 1;
	        num_SAT_calls_ += 1;
        }
        inline void count_total_time_start ()
        {
            total_begin_ = high_resolution_clock::now();
        }
        inline void count_total_time_end ()
        {
            total_end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(total_end_ - total_begin_) .count()/1000.0;
	        time_total_ += duration;
        }
        inline void count_model_construct_time_start ()
        {
            model_begin_ = high_resolution_clock::now();
        }
        inline void count_model_construct_time_end ()
        {
            model_end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(model_end_ - model_begin_) .count()/1000.0;
	        time_model_construct_ += duration;
        }
        inline void count_reduce_uc_SAT_time_start ()
        {
            begin_ = high_resolution_clock::now();
        }
        inline void count_reduce_uc_SAT_time_end ()
        {
            end_ = high_resolution_clock::now();
	        auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        time_reduce_uc_SAT_calls_ += duration;
	        time_SAT_calls_ += duration;
	        num_reduce_uc_SAT_calls_ += 1;
	        num_SAT_calls_ += 1;
        }
        inline void count_orig_uc_size (int size)
        {
            orig_uc_size_ += size;
        }
        inline void count_reduce_uc_size (int size)
        {
            reduce_uc_size_ += size;
        }
        inline void count_clause_contain_time_start ()
        {
        	contain_before_ = high_resolution_clock::now();
        }
        inline void count_clause_contain_time_end ()
        {
        	clock_high contain_end_ = high_resolution_clock::now();
        	auto duration = duration_cast<milliseconds>(contain_end_ - contain_before_) .count()/1000.0;
	        time_clause_contain_ += duration;
        	num_clause_contain_ += 1;
        }
        inline void count_state_contain_time_start ()
        {
        	begin_ = high_resolution_clock::now();
        }
        inline void count_state_contain_time_end ()
        {
        	end_ = high_resolution_clock::now();
        	auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	        time_state_contain_ += duration;
        	num_state_contain_ += 1;
        }
        // inline void count_detect_dead_state_time_start ()
        // {
        //     begin_ = high_resolution_clock::now();
        // }
        // inline void count_detect_dead_state_time_end ()
        // {
        //     end_ = high_resolution_clock::now();
	    //     auto duration = duration_cast<milliseconds>(end_ - begin_) .count()/1000.0;
	    //     time_detect_dead_state_SAT_calls_ += duration;
	    //     num_detect_dead_state_SAT_calls_ += 1;
        // }
        // inline void count_detect_dead_state_success ()
        // {
        //     num_detect_dead_state_success_ += 1;
        // }
        inline void count_converting_lits_start()
        {
            converting_begin_ = high_resolution_clock::now();
        }
        inline void count_converting_lits_end()
        {
            clock_high converting_end_= high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(converting_end_ - converting_begin_) .count()/1000.0;
            time_converting_lits += duration;
        }

        inline void count_before_car_start()
        {
            before_begin_ = high_resolution_clock::now();
        }
        inline void count_before_car_end()
        {
            clock_high before_end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(before_end - before_begin_) .count()/1000.0;
            time_before_car += duration;
        }  

        inline void count_for_car_start()
        {
            for_begin_ = high_resolution_clock::now();
        }
        inline void count_for_car_end()
        {
            clock_high for_end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(for_end - for_begin_) .count()/1000.0;
            time_for_car += duration;
        }  

        inline void count_subgoal_start()
        {
            subgoal_begin_ = high_resolution_clock::now();
        }
        inline void count_subgoal_end()
        {
            clock_high subgoal_end_ = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(subgoal_end_ - subgoal_begin_) .count()/1000.0;
            time_subgoal_ += duration;
        }  
        inline void count_maingoal_start()
        {
            maingoal_begin_ = high_resolution_clock::now();
        }
        inline void count_maingoal_end()
        {
            clock_high maingoal_end_ = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(maingoal_end_ - maingoal_begin_) .count()/1000.0;
            time_maingoal_ += duration;
            maingoal_begin_ = maingoal_end_;
        }  

        inline void count_after_car_start()
        {
            after_begin_ = high_resolution_clock::now();
        }
        inline void count_after_car_end()
        {
            clock_high after_end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(after_end - after_begin_) .count()/1000.0;
            time_after_car += duration;
        }  
        inline void count_clean_start()
        {
            clean_begin_ = high_resolution_clock::now();
        }
        inline void count_clean_end()
        {
            clock_high clean_end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(clean_end - clean_begin_) .count()/1000.0;
            time_cleaning += duration;
        }  

    public:
        int num_SAT_calls_;
        double time_SAT_calls_;
        int num_main_solver_SAT_calls_;
        double time_main_solver_SAT_calls_;
        int num_inv_solver_SAT_calls_;
        double time_inv_solver_SAT_calls_;
        int num_start_solver_SAT_calls_;
        double time_start_solver_SAT_calls_;
        double time_total_;
        double time_model_construct_;
        int num_reduce_uc_SAT_calls_;
        double time_reduce_uc_SAT_calls_;
        double compress_reduce_uc_ratio_;
        int orig_uc_size_;
        int reduce_uc_size_;
        
        int num_clause_contain_;
        int num_state_contain_;
        double time_clause_contain_;
        double time_state_contain_;
        double time_converting_lits;
        clock_high converting_begin_;
        // int num_detect_dead_state_SAT_calls_;
        double time_detect_dead_state_SAT_calls_;


        
        int num_detect_dead_state_success_;
        int converge_total;
        int converge_implied;

public:
        int conv_second_solve_acc=0;
        // accumulation of time spent in calculation.
        int conv_calc_acc = 0;

public:
        int nInAss;
        int nInAss0_succeed;
        int nInAss1_succeed;
        int nInAss2_succeed;
        clock_high global_start;
private: 
        clock_high contain_before_;
        clock_high begin_, end_;
        clock_high total_begin_, total_end_;
        clock_high model_begin_, model_end_;

        clock_high before_begin_;
        double time_before_car;

        clock_high after_begin_;
        double time_after_car;

        clock_high subgoal_begin_;
        double time_subgoal_;

        clock_high maingoal_begin_;
        double time_maingoal_;

        clock_high clean_begin_;
        double time_cleaning;

        clock_high for_begin_;
        double time_for_car;
};



}

#endif

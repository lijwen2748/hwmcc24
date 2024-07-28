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



#ifndef carChecker_H
#define carChecker_H

#include "data_structure.h"
#include "invsolver.h"
#include "startsolver.h"
#include "mainsolver.h"
#include "newpartialsolver.h"
#include "model.h"
#include <assert.h>
#include "utility.h"
#include "statistics.h"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <fstream>
#include <map>

namespace car
{
	extern Statistics stats; // defined in main.cpp
	extern bool verbose_;	 // defined in main.cpp
	extern Model *model;	 // defined in main.cpp
	extern int storage_id;
	class Checker;
	extern Checker ch;

	class Checker
	{
	public:
		/**
		 * @brief Construct a new Checker object
		 * 
		 * @param model aiger model to be checked
		 * @param out 
		 * @param trail_out where to output the search trail (in text)
		 * @param dot_out where to output the search tree 
		 * @param dive_out where to output the dive graph
		 * @param enable_dive whether to output the dive graph
		 * @param forward the search direction
		 * @param evidence whether to print the CEX if unsafe
		 * @param index_to_check the index of property to check. At present, only one property is allowed
		 * @param enable_bi whether to enable bi-direction check. It's to be deprecated maybe.
		 */
		Checker(Model *model,std::ostream &out,std::ofstream &trail_out,std::ofstream &dot_out,std::ofstream &dive_out, bool enable_dive, bool forward = true, bool evidence = false, int index_to_check = 0, bool enable_bi=false);

		Checker(int inter_cnt, Model *model,std::ostream &out,std::ofstream &trail_out,std::ofstream &dot_out,std::ofstream &dive_out, bool enable_dive, bool forward = true, bool evidence = false, int index_to_check = 0, bool enable_bi=false);
		
		/**
		 * @brief Destroy the Checker object
		 * delete all the solvers.
		 */
		~Checker();

		// entrance for CAR checking
		bool check();

	private:
		// entrance for CAR
		bool car();

		/**
		 * @brief Check for immediate safe or unsafe
		 * 
		 * @param out where to print
		 * @param res the check result
		 * @return true : Bad == True / False
		 * @return false 
		 */
		bool trivialCheck(bool& res);

		/**
		 * @brief Searching procedure of bi-car. Enumerate states in U and use the Osequence as the guide.
		 * 
		 * @param U the U sequence to enumerate. 
		 * @param O the O sequence with a singleton (a state that is picked from the U sequence) making up its level 0.
		 * @param forward whether the Transformation relationship should be used forward or backward
		 * @return true : successfully reached O[0]. which means a cex is found.
		 * @return false : all states in present U has been checked. No cex is found.
		 */
		bool trySAT(Usequence &U, Osequence *O, bool forward, bool& safe_reported);
		
	public:
		/**
		 * @section Preprocessing technique
		 * 
		 */
		
		std::vector<Osequence> uc_knowledge;
		Osequence cooked_knowledge;
		#ifdef PP_LIMIT_TIME
			bool isppnow=true;
		#else
			bool isppnow = false;
		#endif
		bool ppstoped=false;
		#if defined(INTER) && !defined(PP_LIMIT_TIME) // a fixed inter cnt.
			#ifdef INTER_CNT
			int inter_cnt = INTER_CNT;
			#else
			int inter_cnt = 1;
			#endif // INTER_CNT
		#else
			int inter_cnt = 0;
		#endif
		#if defined(ROTATE) 
			bool rotate_enabled = true;
		#else
			bool rotate_enabled = false;
		#endif
		
		int sat_counter;
		clock_high sat_timer;

		inline void set_inter_cnt(int cnt){inter_cnt = cnt;}
		inline int get_inter_cnt(){return inter_cnt;}
		inline void set_rotate(bool rtt){rotate_enabled = rtt;}
		inline int get_rotate(){return rotate_enabled;}

		/**
		 * @brief extract the knowledge from prior preprocessing phases heavily
		 * 
		 */
		void cook();
		
		/**
		 * @brief extract the knowledge from prior preprocessing phases heavily
		 * 
		 */
		void cook_light();

		/**
		 * @brief pick the next pp strategy
		 * 
		 */
		void pick_next_pp();

	public:
		// we need to record whether it used another UC in the history.
		// level -> array[INT_MAX]
		// we use the bit to record whether it pushed another UC.
		std::unordered_map<int, unsigned short> conv_record;

	private:
		Model *model_; 
		bool evidence_;
		bool backward_first = true;
		bool enable_bi_check = false;
		int bad_;
		StartSolver *bi_start_solver;

		// mapping from state to its corresponding o sequence.
		std::unordered_map<const State*,Osequence*> SO_map;
		// the states blocked.
		std::unordered_set<State *> blocked_states;
		// the main solver shared.
		MainSolver *bi_main_solver;
		// the partial solver shared.
		PartialSolver *bi_partial_solver;
		// count of blocked states.
		int blocked_count=0;

		// the map from O sequence to its minimal_level
		std::unordered_map<const Osequence*, int> fresh_levels;
		Usequence Uf, Ub; //Uf[0] is not explicitly constructed 
		Osequence Onp, OI;
		// used in picking state randomly
		std::vector<std::pair<State *, int>> Uset;

		int stuck_counter = 0;
	
	public:
		/**
		 * @section Counter Example 
		 * 
		 */

		inline Usequence& whichU()
		{
			return backward_first ? Ub : Uf;
		}
		
		inline Usequence& otherU()
		{
			return backward_first ? Uf : Ub;
		}

		// record the prior state in this trail. This will be used in counter example printing.
		std::map<State *, State*> prior_in_trail_f;
		std::map<State *, State*> prior_in_trail_b;
		inline std::map<State *, State*>& whichPrior()
		{
			return backward_first ? prior_in_trail_b : prior_in_trail_f;
		}

		State* counter_start_f = nullptr;
		State* counter_start_b = nullptr;
		inline State*& whichCEX()
		{
			return backward_first ? counter_start_b : counter_start_f;
		}
		inline State*& otherCEX()
		{
			return backward_first ? counter_start_f : counter_start_b;
		}
		std::ostream &out;
		std::ostream &trail_out;

		/**
		 * @brief print evidence to out.
		 * 
		 * @param out 
		 */
		void print_evidence() const;

	private:
		/**
		 * @section Clear duties
		 * 
		 */
		
		// the states to be cleared
		std::unordered_set<State*> clear_duties;
		// add into clear duty, waiting for clear in the end.
		inline void clear_defer(State* s){clear_duties.insert(s);};
		// the main solver to be cleared
		std::unordered_set<MainSolver*> clear_duties_mainsolver;
		// add into clear duty, waiting for clear in the end.
		inline void clear_defer(MainSolver* s){clear_duties_mainsolver.insert(s);};

	private:
		/**
		 * @section Basic methods
		 * 
		 */
		/**
		 * @brief Create a O with given state object as its level 0.
		 * 
		 * @param s 
		 * @return Osequence& 
		 */
		Osequence* createOWith(State *s);


		/**
		 * @brief iteratively pick state from the given sequence. 
		 * @return State*  
		 * 	then this state will be marked as visited at this round.
		 * @return nullptr every state has been tried.
		 *  then all the markers will be cleaned. Therefore, this method can be reused for another round.
		 * @post Anytime it ends the iterative round earlier(before nullptr), it must has found the counter example in trySAT. Therefore, there will be no picking in the future. 
		 */
		State *pickState(Usequence &);
		State *pickState(Usequence &, int &level);
		State *pickStateRandom(Usequence &, int &level);
		
		/**
		 * @brief Get the partial state with assignment s. This is used in forward CAR.
		 * 
		 * @param s 
		 * @param prior_state 
		 * @return State* 
		 */
		State* get_partial_state(Assignment &s, const State* prior_state);


		/**
		 * @brief Get the solution from SAT solver.
		 * 
		 * @return State* : the state representing the solution, which is to be added to the U sequence.
		 */
		State* getModel(MainSolver*);
		State* getModel(MainSolver*,State*);
		
		/**
		 * @brief Update U sequence, and push into cex vector
		 * 
		 * @param level 
		 * @return whether it is already seen at a lower level.
		 */
		bool updateU(Usequence&, State*, int level, State* prior_in_trail);

		/**
		 * @brief Update O sequence
		 * 
		 * @param O 
		 * @param dst_level 
		 * @param Otmp 
		 * @param safe_reported 
		 */
		void updateO(Osequence *O, int dst_level, Frame &Otmp, bool& safe_reported);

		/**
		 * @brief SAT_Assume(assum, clauses)
		 * 
		 * @return true 
		 * @return false 
		 */
		bool satAssume(MainSolver*, Osequence *O, State*, int, const Frame& Otmp);

		/**
		 * @brief Interface for cleaning.
		 * 
		 */
		void clean();

		/**
		 * @brief When Os finds an invariant, clear Os and erase s from Os.
		 * 
		 * @param s 
		 * @param Os 
		 */
		void clearO(State* s, Osequence *Os);

		/**
		 * @brief Block this state in main solver forever.
		 * Because it is blocked forever, there is no need for a flag.
		 */
		void blockStateForever(const State*);

		/**
		 * @brief first create a clause with target uc and flag of the corresponding O[dst], then add this clause to main solver or start solver, depending on the level.
		 * 
		 * @param uc 
		 * @param O
		 * @param dst_level
		 */
		void addUCtoSolver(Cube& uc, Osequence *O, int dst_level,Frame& Otmp);

		/**
		 * @brief init special sequences: Uf, Ub, Oi, Onp
		 */
		bool initSequence(bool& res);

		/**
		 * @brief Use start solver to get a state. Usually in ~p.
		 * 
		 * @param start_solver 
		 * @return State* 
		 */
		State* enumerateStartStates(StartSolver *start_solver);

		/**
		 * @brief Check SAT_ASSUME(I, ~P).
		 * 
		 * to generate uc from init
		 * ~p should be in ~uc
		 * use uc to initialize O[0] is suitable. 
		 * 
		 */
		bool immediateCheck(State *from, int target, bool &res, Frame &O0);

		/**
		 * @brief Check whether this in O[0] is actually a CEX.
		 * 
		 * @param from 
		 * @param O 
		 * @return true 
		 * @return false 
		 */
		bool lastCheck(State* from, Osequence* O);



		/**
		 * @brief Whether this state is blocked in this O frame, namely O[frame_level] (or Otmp, if frame_level == O.size)
		 * 
		 * @param s 
		 * @param frame_level 
		 * @param O 
		 * @param Otmp 
		 * @return true 
		 * @return false 
		 */
		bool blockedIn(const State* s, const int frame_level, Osequence *O, Frame & Otmp);

		/**
		 * @brief Use `blocked in` to iterate, from min to max, to find the minimal level where this state is not blocked.
		 * 
		 * @param s 
		 * @param min 
		 * @param max 
		 * @param O 
		 * @param Otmp 
		 * @return int 
		 */
		int minNOTBlocked(const State*s, const int min, const int max, Osequence *O, Frame &Otmp);

		/**
		 * @brief Remove state from U sequence
		 * 
		 * @param s 
		 * @param U 
		 */
		void removeStateFrom(State*s, Usequence&U);

	public:
		/**
		 * @section output trail and draw graph
		 * 
		 */
		std::ofstream &dot_out;
		void print_flags(std::ostream &out);
		/**
		 * @brief Helper function to print SAT query.
		 * 
		 * @param solver 
		 * @param s 
		 * @param o 
		 * @param level 
		 * @param res 
		 * @param out 
		 */
		void print_sat_query(MainSolver *solver, State* s, Osequence *o, int level, bool res, std::ostream& out);
		void draw_graph();
		void print_U_shape();
		std::vector<int> spliter;
		std::set<int> blocked_ids;
		std::vector<int> blocked_counter_array;
		// count of tried before
		int direct_blocked_counter = 0;

	private:
		/**
		 * @section rotation technique
		 * 
		 */
		// corresponding to O[i]
		std::vector<Cube> rotates;
		// corresponding to Otmp
		Cube rotate;

	private:
		/**
		 * @section use failed states to score
		 * 
		 */
		std::vector<std::unordered_map<int,int>> score_dicts;
		std::unordered_map<int,int> score_dict; 
		std::unordered_map<int,int> decayStep;
		std::unordered_map<int,int> decayCounter;
	
	public:
		/**
		 * @section drawing diving-graph in O-level.
		 * 
		 */
		
		bool enable_dive;
		std::ofstream &dive_out;
		// whether this is a fresh O frame, which means uc in it has just been updated and never does a SATAssume take place.
		std::vector<bool> O_level_fresh;
		std::vector<int> O_level_fresh_counter;
		// the last state id to visit this level. 
		std::vector<int> O_level_repeat;
		std::vector<int> O_level_repeat_counter;
		int dive_counter = 0;
		int dive_maxx = 0;
		State* dive_last_state = nullptr;
		std::map<int,std::string> colors = {
			// black
			{0,"#000000"},
			// red
			{1,"#f00003"},
			// green
			{2,"#008000"},
			// purple
			{3,"#800080"}
		};
		inline void dive_draw_wedge(State* from, int from_level, State*to, int to_level, int color, int wedge_label)
		{
			dive_out<<"// "<<from->id<<"/"<<from_level<<" , "<<int(to? to->id : -1)<<"/"<<to_level<<std::endl;
			if(from == to)
			// backtrack
			{
				if(from != dive_last_state)
				{
					// some state is picked, and fails at once
					if(dive_counter!=1)
						dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter<<"[style=dotted] ;"<<std::endl;
					dive_maxx++;
					dive_draw_vextex(from,from_level, dive_maxx, false);
				}
				dive_maxx++;
				dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter;
				dive_out<<"[style=dashed";
				dive_out<<", color=\""<<colors[color]<<"\"";
				if(wedge_label >= 1)
					dive_out<<", label=\""<<wedge_label<<"\", fontcolor=\""<<colors[color]<<"\"";
				dive_out<<"] ;"<<std::endl;
				dive_draw_vextex(to,to_level, dive_maxx, false);
				dive_last_state = to;
			}
			else if(!to)
			// from is left out
			{
				// if(from != dive_last_state)
				// {
				// 	dive_maxx++;
				// 	dive_counter++;
				// 	dive_draw_vextex(from,from_level, dive_maxx, false);
				// }
				// else
				// {
				// 	dive_draw_vextex(from,from_level, dive_maxx, true);
				// 	dive_counter++;
				// }

				if(from != dive_last_state)
				{
					if(dive_counter!=1)
						dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter<<"[style=dotted] ;"<<std::endl;
					dive_maxx++;
					// dive_counter++;
					dive_draw_vextex(from,from_level, dive_maxx, false);
				}
				dive_maxx++;
				
				dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter<<"[style=dotted";
				dive_out<<", color=\""<<colors[color]<<"\"";
				if(wedge_label >= 1)
					dive_out<<", label=\""<<wedge_label<<"\", fontcolor=\""<<colors[color]<<"\"";
				dive_out<<"] ;"<<std::endl;
				
				dive_out<< "s" << dive_counter << "[label=OUT, style=dotted,  pos=\""<<dive_maxx<< ","<<from_level+2<<"!\"]"<<std::endl;

				

				dive_last_state=nullptr;
			}
			else 
			{
				// continue to try
				if(from != dive_last_state)
				{
					if(dive_counter!=1)
						dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter<<"[style=dotted] ;"<<std::endl;
					dive_maxx++;
					dive_draw_vextex(from,from_level, dive_maxx, false);
				}
				// else : another state to try
				dive_out <<"s"<< dive_counter++ <<" -> "<< "s"<<dive_counter;
					dive_out<<"[color=\""<<colors[color]<<"\"";
				if(wedge_label >= 1)
					dive_out<<", label=\""<<wedge_label<<"\", fontcolor=\""<<colors[color]<<"\"";
				dive_out<<"] ;"<<std::endl;
				dive_draw_vextex(to,to_level, dive_maxx, false);
				dive_last_state = to;
			}


		}

		inline void dive_draw_head()
		{
			dive_out<<"digraph G {"<<std::endl;
			dive_counter = 1;
			dive_out<<
			"\
			tl11[style=\"invis\", pos=\"0,8!\"]; \n\
			tl11 -> tl12[style=\"invis\",label=\"legend\"];\n\
			tl12[style=\"invis\", pos=\"3,8!\"]; \n\
			\n\
			tl21[style=\"invis\", pos=\"0,7!\"]; \n\
			tl21 -> tl22[color=\""<<colors[0].c_str()<<"\",label=\"basic\"];\n\
			tl22[style=\"invis\", pos=\"3,7!\"]; \n\
			\n\
			tl31[style=\"invis\", pos=\"0,6!\"]; \n\
			tl31 -> tl32[color=\""<<colors[1].c_str()<<"\",label=\"tried before\"];\n\
			tl32[style=\"invis\", pos=\"3,6!\"]; \n\
			\n\
			tl41[style=\"invis\", pos=\"0,5!\"]; \n\
			tl41 -> tl42[color=\""<<colors[2].c_str()<<"\",label=\"same O frame\"];\n\
			tl42[style=\"invis\", pos=\"3,5!\"]; \n\
			\n\
			tl51[style=\"invis\", pos=\"0,4!\"]; \n\
			tl51 -> tl52[color=\""<<colors[3].c_str()<<"\",label=\"same state to this level\"];\n\
			tl52[style=\"invis\", pos=\"3,4!\"]; \n\
			"<<std::endl;
		}

		inline void dive_mark_bad()
		{
			dive_out<<"s"<<dive_counter<<"[label = BAD, color=blue, pos=\"" <<dive_maxx<<",-1!\"]"<<std::endl;
		}

		inline void dive_draw_tail()
		{
			dive_out<<"}"<<std::endl;
		}

		inline void dive_draw_vextex(State*s, int level,int posx,bool dash)
		{
			dive_out<< "s" << dive_counter << "[label="<<s->id<<", pos=\""<<posx<< ","<<level<<"!\"";
			if(dash)
				dive_out<<",style=dashed] ;"<<std::endl;
			else
				dive_out<<"]; "<<std::endl;
		}


	public:
		std::vector<std::pair<int,int>> longest_uc_index;
		std::vector<std::pair<int,int>> shortest_uc_index;
	
	private:
		/**
		 * invariant section
		 *
		 */

		bool InvTrySat(State* missionary, Usequence &U, Osequence *O, int level,bool& safe_reported, InvSolver * inv_solver);
		
		/**
		 * @brief Check whether there exists an invariant in this sequence. Which means the center state of this Osequence is not reachable, and should be later blocked
		 * 
		 * @param O the sequence to be checked
		 * @return true : invariant is found
		 * @return false : no invariant is found
		 */
		bool InvFound(Osequence *O);

		bool InvFoundAt(Osequence& O, int check_level,int minimal_update_level, InvSolver *inv_solver);

	};


	namespace inv
	{
		void InvAddOR(Frame& frame, int level, InvSolver* inv_solver_);

		void InvAddAND(Frame& frame, int level, InvSolver* inv_solver_);

		void InvRemoveAND(InvSolver* inv_solver_, int level);

		bool InvFoundAt(Fsequence &F_, const int frame_level,  InvSolver *inv_solver_, int minimal_update_level_);
	}

}


#endif

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
#include "newpartialsolver.h"
#include <vector>
#include <iostream>
#include <stack>
#include <functional>
#include "utility.h"
#include "statistics.h"
#include <algorithm>
#include "DEBUG_PRINTING.h"
#include <unordered_set>
#include <queue>
#include <tuple>
#include <chrono>
using namespace std;
using namespace std::chrono;
using clock_high = time_point<high_resolution_clock>;

namespace car
{
	// increase one each time. monotonous
	int storage_id = 0;
	static vector<Cube> reorderAssum(vector<Cube> inter, const Cube& rres, const Cube& rtmp);

	Checker::Checker(Model *model,std::ostream &out,std::ofstream & trail_out,std::ofstream & dot_out, std::ofstream &dive_out, bool enable_dive, bool forward, bool evidence, int index_to_check,bool enable_bi):model_(model),out(out),dot_out(dot_out), dive_out(dive_out), enable_dive(enable_dive), evidence_(evidence),enable_bi_check(enable_bi),trail_out(trail_out)
	{
		// TODO: use propagate to accelerate
		backward_first = !forward;
		bi_main_solver = new MainSolver(model);
		bi_partial_solver = new PartialSolver(model);
		// TODO: extend to multi-properties.
		bad_ = model->output(index_to_check);
		bi_start_solver = new StartSolver(model,bad_,true);
		if(enable_dive)
		{
			assert(dive_out.is_open());
			dive_draw_head();
		}
		rotates.clear();
		rotate.clear();
		score_dicts.clear();
		score_dict.clear();
	}

	
	Checker::Checker(int inter_cnt, Model *model,std::ostream &out,std::ofstream & trail_out,std::ofstream & dot_out, std::ofstream &dive_out, bool enable_dive, bool forward, bool evidence, int index_to_check,bool enable_bi):model_(model),out(out),dot_out(dot_out), dive_out(dive_out), enable_dive(enable_dive), evidence_(evidence),enable_bi_check(enable_bi),trail_out(trail_out)
	{
		// TODO: use propagate to accelerate
		backward_first = !forward;
		bi_main_solver = new MainSolver(model);
		bi_partial_solver = new PartialSolver(model);
		// TODO: extend to multi-properties.
		bad_ = model->output(index_to_check);
		bi_start_solver = new StartSolver(model,bad_,true);
		if(enable_dive)
		{
			assert(dive_out.is_open());
			dive_draw_head();
		}

		rotates.clear();
		rotate.clear();
		score_dicts.clear();
		score_dict.clear();
		set_inter_cnt(inter_cnt);
	}

	Checker::~Checker()
	{
		clean();
		if(bi_main_solver)
		{
			delete bi_main_solver;
			bi_main_solver = NULL;
		}
		if(bi_start_solver)
		{
			delete bi_start_solver;
			bi_start_solver = NULL;
		}
		if(bi_partial_solver)
		{
			delete bi_partial_solver;
			bi_partial_solver = NULL;
		}
	}

	bool Checker::check()
	{
		bool res;
		if(trivialCheck(res))
		{
			return res ? true : false; 
		}

		res = car();
		if(res && ppstoped)
		{
			// should try the next strategy.
			return true;
		}

		LOG("End");
		if(evidence_)
		{
			do
			{
				if (res)
				{
					out << "0" << endl;
					out << "b0" << endl;
					out << "." << endl;
					break;
				}

				print_evidence();


				print_U_shape();
				if(dot_out.is_open())
				{
					draw_graph();
				}
				if(dive_out.is_open())
				{
					dive_mark_bad();
					dive_draw_tail();
				}
			}
			while (false);
		}
		
		return res;
	}

	bool Checker::car()
	{
		bool res = false;
		if(initSequence(res))
			return res;

		if(isppnow)
		{
			sat_counter = 0;
			sat_timer = high_resolution_clock::now();
		}

		/**
		 * As to backward search,
		 * @post Ob, Ub are both initialized.
		 * uc of UNSAT(I, ~P) is inserted into O[0].
		 * O[0] is inserted into MainSolver. 
		 */

		while (true)
		{
			bool direction = !backward_first;
			LOG((direction ? "F" : "B"));
			Usequence &U = direction ? Uf : Ub;
			
			if(enable_bi_check)
			{	
				Usequence &V = direction ? Ub : Uf;
				# ifdef PICK_GUIDE_RANDOM
				while(State *s = pickStateRandom (V))
				#else
				while(State *s = pickState(V)) // this state works as the target
				#endif
				{
					if(blocked_states.find(s)!=blocked_states.end()) continue;
					otherCEX() = s;
					LOG("Bi-Target:"<<s->id);
					Osequence *Os = createOWith(s);
					if (trySAT(U, Os, direction,res))
					{
						if(ppstoped)
						{
							break;
						}
						if(res)
						{
							blocked_states.insert(s);
							// block in solver
							blockStateForever(s);
							// clear Os and remove s from state -> Osequence map;
							clearO(s,Os);
							removeStateFrom(s,U);
							continue;
						}
						else
							return res; // unsafe if not safe-reported.
					}
					if (InvFound(Os))
					{
						// block this state
						blocked_states.insert(s);
						// block in solver
						blockStateForever(s);
						// clear Os and remove s from state -> Osequence map;
						clearO(s,Os);
						removeStateFrom(s,U);
					}
				}
			}
			
			Osequence &O = direction ? OI : Onp;
			LOG("Bi-Target:"<< (direction ? "I" : "~P"));

			if (trySAT(U, &O, direction,res))
			{
				// if this PP phase ends, but has not reached the whole end.
				if(ppstoped)
				{
					return true;
					// pick_next_pp();
					// continue;
				}
				return res;
			}

			// during pp, no need to check this. Only main procedure need to check this.
			#ifndef INCOMPLETE
			if(!isppnow)
				if (InvFound(&O))
				return true;
			#endif // !INCOMPLETE

			if(enable_bi_check)
			{
				static int counter = 0;
				if(counter % 5 == 0 || counter % 5 == 1)
				backward_first = !backward_first;	
				++counter;
			}
		}

		// dead code. Should not reach
		return false;
	}

	bool Checker::trivialCheck(bool& res)
	{
		const Model* model = model_;
		// FIXME: fix for multiple properties. 
		if (bad_ == model->true_id())
		{
			out << "1" << endl;
			out << "b" << "0" << endl;
			if (evidence_)
			{
				// print init state
				// FIXME: fix for latch inits.
				for(int i = 0; i < model->num_latches(); ++i)
					out<< "0";
				out<<endl;
				// print an arbitary input vector
				for (int j = 0; j < model->num_inputs(); j++)
					out << "0";
				out << endl;
			}
			out << "." << endl;
			if (verbose_)
			{
				cout << "return SAT since the output is true" << endl;
			}
			res = true;
			return true;
		}
		else if (bad_ == model->false_id())
		{
			out << "0" << endl;
			out << "b" << endl;
			out << "." << endl;
			if (verbose_)
			{
				cout << "return UNSAT since the output is false" << endl;
			}
			res = false;
			return true;
		}
		return false;
	}

	bool Checker::trySAT(Usequence &U, Osequence *O, bool forward, bool&safe_reported)
	{
		// NOTE: can eliminate initialization.
		safe_reported = false;
		Frame Otmp;
		if(!isppnow && O->size()< cooked_knowledge.size())
		{
			Otmp = cooked_knowledge[O->size()];
		}
		MainSolver *main_solver = bi_main_solver;
		// FIXME: only reset when meeting ~p.
		// move into pickState();
		bi_start_solver->reset();
		int state_level = -1;
		/**
		 * this procedure is like the old car procedure, but the Osequence is not bound to be OI or Onegp.
		 * @param missionary the state to be checked 
		 */

		#ifndef FRESH_U
			Usequence Uold = U;
		#else
			Usequence &Uold= U;
		#endif

		while(State* missionary = pickState(Uold,state_level))
		{
			LOG("Pick "<<missionary->id);
			/**
			 * build a stack <state, from_level, target_level>
			 */
			stuck_counter = 0;
			CONTAINER stk;
			stk.push(item(missionary,state_level, O->size()-1));
			while (!stk.empty())
			{
				State *s; int dst, src;
				std::tie(s, src, dst) = stk.top();
				LOG("Try " << s->id << " " << dst);
				// bi_main_solver->print_last_3_clauses();
				if (blockedIn(s, dst + 1, O, Otmp))
				{
					stk.pop();
					LOG("Tried before");
					direct_blocked_counter++;
					blocked_ids.insert(s->id);
					
					int new_level = minNOTBlocked(s, dst + 2, O->size()-1, O, Otmp);
					if (new_level <= O->size())
					{
						stk.push(item(s, src, new_level-1));
						LOG("Again " << s->id << " " << dst << " " << new_level-1);
						DIVE_DRAW(s,dst+1, s, new_level,1,0);
					}
					else DIVE_DRAW(s,dst+1,nullptr,0,1,0);
					continue;
				}
				
				if(isppnow)
				{
					sat_counter++;

					#ifdef PP_LIMIT_CNT
					if(sat_counter > PP_LIMIT_CNT)
					{
						ppstoped=true;
					}
					#endif // SAT_LIMIT_CNT

					#ifdef PP_LIMIT_TIME
					if(duration_cast<milliseconds>(high_resolution_clock::now() - sat_timer).count() > PP_LIMIT_TIME * 1000)
					{
						ppstoped=true;
					}
					#endif
					
					if(ppstoped)
					{	
						return true;
					}
				}

				if (satAssume(bi_main_solver, O, s, dst, Otmp))
				{
					LOG("Succeed");
					if (dst == -1)
						return true;

					State *tprime = getModel(bi_main_solver);
					LOG("Get " << tprime->id << " " << dst);

				#ifdef ORDERED_U
					updateU(U, tprime, dst, s);
				#else
					updateU(U, tprime, src + 1, s);
				#endif

					int new_level = minNOTBlocked(tprime, 0, dst-1, O, Otmp);
					if (new_level <= dst) // if even not one step further, should not try it
					{	
						stk.push(item(tprime, src + 1, new_level - 1));
						LOG("Jump " << dst << " " << new_level-1);
						// if(dst != new_level)
						// 	cerr<<"dst: "<<dst<<", new_level : "<<new_level<<endl;
						DIVE_DRAW(s,dst+1, tprime, new_level,(O_level_repeat[dst] != s->id ? 0 : 3),O_level_repeat[dst] == s->id ? O_level_repeat_counter[dst]+1 : 0);
					}
					if(dst >=0)
					{
						if(O_level_repeat[dst] == s->id)
							O_level_repeat_counter[dst]++;
						else
						{
							O_level_repeat[dst] = s->id;
							O_level_repeat_counter[dst] = 1;
						}
					}
				}
				else
				{
					LOG("Fail ");
					stk.pop();

					updateO(O, dst, Otmp, safe_reported);
					if (safe_reported)
						return true;

					int new_level = minNOTBlocked(s, dst + 2, O->size()-1, O, Otmp);
					if (new_level <= O->size())
					{
						stk.push(item(s, src, new_level-1));
						LOG("Again " << s->id << " " << dst << " " << new_level-1);
						DIVE_DRAW(s,dst+1, s, new_level,((dst==-1 || O_level_fresh[dst]) ? 0 : 2),(dst==-1 || O_level_fresh[dst]) ? 0 : (O_level_fresh_counter[dst]));
					}
					else {
						DIVE_DRAW(s,dst+1,nullptr,0,((dst==-1 || O_level_fresh[dst]) ? 0 : 2),(dst==-1 || O_level_fresh[dst]) ? 0 : (O_level_fresh_counter[dst]));
					}
					// this is now tried~
					if(dst >= 0)
					{
						if(O_level_fresh[dst])
						{
							O_level_fresh[dst] =false;
							O_level_fresh_counter[dst] = 1;	
						}
						else{
							O_level_fresh_counter[dst]++;	
						}
					}
				}

			}
		}

		// this is same with extend_F_sequence in old CAR.
		O->push_back(Otmp);
		O_level_fresh.push_back(true);
		O_level_fresh_counter.push_back(0);
		O_level_repeat.push_back(-2);
		O_level_repeat_counter.push_back(0);

		spliter.push_back(State::next_id_);
		blocked_counter_array.push_back(direct_blocked_counter);
		direct_blocked_counter = 0;
		#ifdef ROTATE
		if(rotate_enabled)
			rotates.push_back(rotate);
		#endif //ROTATE
		#ifdef SCORE
		score_dicts.push_back(score_dict);
		#endif //SCORE
		bi_main_solver->add_new_frame(Otmp,O->size()-1,O,forward);
		PRINTIF_PROOF();
		return false;
	}
	
	void Checker::cook_light()
	{
		for(auto dish:uc_knowledge){
			cooked_knowledge.resize(1);			
			for(auto uc: dish[0])
			{
				if(backward_first)
					Onp[0].push_back(uc);
				else
					OI[0].push_back(uc);
			}
		}
	}

	void Checker::cook(){
		for(auto dish:uc_knowledge){
			if(cooked_knowledge.size() < dish.size())
				cooked_knowledge.resize(dish.size());
			for(int i = 0; i<dish.size();++i)
			{
				for(auto uc: dish[i])
				{
					int cook_thresh = 2;
					if(uc.size()<cook_thresh)
						// no need for implication analysis, will be changed later.
						cooked_knowledge[i].push_back(uc);
				}
			}
		}
	}

	void Checker::pick_next_pp()
	{
		// ##############  restore section  ##############
		cout<<"enter pp pick at : "<<duration_cast<milliseconds>(high_resolution_clock::now() - stats.global_start).count()/1000 <<endl;
		clock_high enter = high_resolution_clock::now();
		delete(bi_main_solver);
		bi_main_solver = new MainSolver(model);
		if(!backward_first)
		{
			delete(bi_start_solver);
			bi_start_solver = new StartSolver(model,bad_,true);
		}
		whichPrior().clear();
		
		// may not need to be cleared, but for memory saving, we do it here.
		for(int i = 0; i< whichU().size();++i)
		{
			auto frame = whichU()[i];
			for(auto s: frame)
			{
				delete s;
				clear_duties.erase(s);
			}
		}
		// only remain the init size
		Ub.resize(0);
		Uf.resize(0);
		Uset.clear();
		fresh_levels.clear();

		if(backward_first)
		{
			uc_knowledge.push_back(Onp);
		}
		else
		{
			uc_knowledge.push_back(OI);
		}
		Onp.clear();
		OI.clear();
		SO_map.clear();
		rotates.clear();
		rotate.clear();

		score_dicts.clear();
		score_dict.clear();

		blocked_ids.clear();
		blocked_states.clear();
		blocked_counter_array.clear();
		bool res;
		initSequence(res);

		// ##############  pick section  ##############
		cout<<"next pp picked"<<endl;

		int maxni=999;
		#ifdef MAXNI
		maxni=MAXNI;
		#endif // MAXNI
		int ni = get_inter_cnt();
		

		if(ni < maxni)
		{
			set_inter_cnt(ni+1);
			if(ni == maxni-1)
			{
				isppnow=false;
			}
		}
		cout<<"I:"<<get_inter_cnt()<<", R:"<<get_rotate()<<endl;
		ppstoped=false;
		sat_counter = 0;
		sat_timer = high_resolution_clock::now();
	}
	
	bool Checker::InvTrySat(State* missionary, Usequence &U, Osequence *O, int level, bool&safe_reported, InvSolver * inv_solver)
	{
		safe_reported = false;
		bool forward = !backward_first;
		// just an empty Otmp is enough.
		Frame Otmp;
		MainSolver *main_solver = bi_main_solver;
		int state_level = -1;

		CONTAINER stk;
		stk.push(item(missionary,state_level, level));
		bool res = false;
		while (!stk.empty())
		{
			State *s; int dst, src;
			std::tie(s, src, dst) = stk.top();
			if (blockedIn(s, dst + 1, O, Otmp))
			{
				stk.pop();
				
				// should not try levels lagrger than original
				int new_level = minNOTBlocked(s, dst + 2, level, O, Otmp);
				if (new_level <= level)
				{
					stk.push(item(s, src, new_level-1));
				}
				continue;
			}

			if (satAssume(bi_main_solver, O, s, dst, Otmp))
			{
				if (dst == -1)
				{
					res = true;
					break;
				}

				State *tprime = getModel(bi_main_solver);
				inv_solver->inv_update_U(U, tprime, src + 1, s);

				int new_level = minNOTBlocked(tprime, 0, dst-1, O, Otmp);
				if (new_level <= dst) // if even not one step further, should not try it
				{	
					stk.push(item(tprime, src + 1, new_level - 1));
					LOG("Jump " << dst << " " << new_level-1);
				}
			}
			else
			{
				stk.pop();

				updateO(O, dst, Otmp, safe_reported);
				// inv solver needs to be updated
				Cube uc = bi_main_solver->get_conflict(!backward_first);
				std::sort(uc.begin(),uc.end(),car::comp);
				inv_solver->add_uc(uc, dst+1,level);

				if (safe_reported)
					return true;

				int new_level = minNOTBlocked(s, dst + 2, level, O, Otmp);
				if (new_level <= level)
				{
					stk.push(item(s, src, new_level-1));
				}
			}
		}

		// We build a tree of states here. But when adding to the opposite U sequence, not all the states are needed. If one state has multiple successors, then there will be some states that have multiple predecessors. It does not help to guide.

		if(res && enable_bi_check)
		{
			std::set<State*> used;
			State* c = whichCEX();
			int other_level = 0; // start from 0
			State* pr = nullptr;
			while(c)
			{
				updateU(otherU(),c,other_level,pr);
				++other_level;
				pr = c;
				c = inv_solver->prior[c];
				used.insert(c);
			}
			inv_solver->prior.clear();
		}
		return res;
	}

	/**
	 * @brief 
	 * 
	 * @pre level less than check_level has been checked before
	 * @param O 
	 * @param check_level 
	 * @return true : invariant found at target level
	 * @return false : invariant does not exists at target level
	 */
	bool Checker::InvFoundAt(Osequence& O, int check_level,int minimal_update_level, InvSolver *inv_solver)
	{
		// a portion of `InvFound()`
		if(check_level < minimal_update_level)
		{
			inv::InvAddOR(O[check_level],check_level, inv_solver);
			return false;
		}
		inv::InvAddAND(O[check_level], check_level, inv_solver);
		
		bool res = !inv_solver->solve_with_assumption();

		#ifdef INV_HEAVY
		while(!res)
		#elif defined(DINV_MEDIUM)
		int max_try = check_level;
		if(!res && --max_try )
		#elif defined(INV_LIGHT)
		int max_try = 1;
		if(!res && max_try--)
		#else
		while(false)
		#endif
		{
			// This solution is a state, that is in O[check_level], but not in any prior level.
			// We should test whether it can really reach the target, namely ~p for backward-CAR.
			// If so, update them into U sequence of the opposite direction
			// If not, refine the O sequence. 
			// it's like try_satisfy, but it differs in that it should keep a new 'U'.
			State* s = inv_solver->get_state(!backward_first); // get the state, which is in Oj but not in O0....Oj-1.
			clear_defer(s);

			Usequence Utmp = {{s}};
			bool safe_reported;
			
			bool s_reachable = InvTrySat(s, Utmp,&O,check_level-1,safe_reported, inv_solver);
			if(s_reachable)
			{
				// opposite U is extended.
				res = false;
				// break;
			}
			else{
				// TODO: add uc implication analysis in inv_solver.
				// refinement of O sequence is done in InvTrySat
				if(safe_reported)
				{
					res = false;
					// break;
				}
				res = !inv_solver->solve_with_assumption();
				#ifdef PRINT_INV
				cout<<"level = "<< check_level <<", inv found ?= "<<res<<", frame size = "<<O[check_level].size()<<endl;
				#endif
			}
		}
		inv::InvRemoveAND(inv_solver,check_level);
		inv::InvAddOR(O[check_level], check_level, inv_solver);		
		return res;
	}

    bool Checker::InvFound(Osequence* o)
	{
		Osequence &O = *o;
		bool res = false;
		// FIXME: Should we reuse inv_solver instead of recreating?
		InvSolver *inv_solver = new InvSolver(model_);
		// FIXED: shall we start from 0 or 1? 
		// 0, to add O[0] into solver.

		/**
		 * @brief About minimal update level: 
		 * 		It is related to specific O sequence. 
		 * 		Each time invariant check is done, minimal update level is reset to the highest level. 
		 * 		Each time a modification of low-level frame will possibly modify this minimal level.
		 * 
		 */
		for(int i = 0 ; i < O.size(); ++i)
		{
			if(InvFoundAt(O,i,fresh_levels[o],inv_solver))
			{
				#ifdef INV_PRINT
				cout<<"invariant found at "<<i<<endl;
				for(int j = 0; j<=i ;++j)
				{
					cout<<"level = "<<j<<endl;
					cout<<"uc := "<<endl;
					for(auto uc:O[j])
					{
						cout<<"(";
						for(int k:uc)
							cout<<k<<", ";
						cout<<")"<<endl;
					}
				}
				#endif
				while(O.size() > i)
					O.pop_back();
				res = true;
				// already found invariant.
				fresh_levels[o] = -1;
				break;
			}
		}
		//NOTE: not O.size()-1. because that level is also checked.
		fresh_levels[o] = o->size();
		delete(inv_solver);
		#ifdef PRINT_INV
		cout<<"END OF ONE ROUND"<<endl<<endl;
		#endif
		return res;
	}

	Osequence* Checker::createOWith(State* s)
	{
		Osequence *o;
		if (SO_map.find(s) != SO_map.end())
		{
			o = SO_map[s];
		}
		else
		{
			// Cube assigns = s->s();
			// Cube neg_ass = negate(assigns);
			Frame f;
			for(auto i: s->s())
				f.push_back({-i});
			o = new Osequence({f});
			SO_map[s] = o;
			fresh_levels[o] = 0;
			// FIXME: When is the right time to add to solver? And when to set the flag?
			bi_main_solver->add_new_frame(f,o->size()-1, o,!backward_first);
		}
		assert(o);
		return o;
	}

	// NOTE: when we try to pick a guide state, it is not used as part of the assumption but the target. Therefore, uc is not generated according to it, and therefore will not be updated.
	// Luckily, we do not use Onp or OI to guide. Therefore, here we have nothing to do with start_solver.
    State *Checker::pickState(Usequence &U) 
    {
		static std::unordered_set<unsigned> _seen;
		// NOTE: here, do not directly use Onp or OI.
		for(int i = U.size()-1; i > 0; --i)
		{
			auto frame = U[i];
			for(auto state_ptr:frame)
			{
				if(state_ptr->is_negp)
				{
					continue;
				}
				else if(_seen.count(state_ptr->id) == 0)
				{
					_seen.insert(state_ptr->id);
					return state_ptr;					
				}
			}
		}
		_seen.clear();
        return nullptr;
    }

	State *Checker::pickStateRandom(Usequence &U, int& level)
    {
		static int index = 0;
		static int maxindex = Uset.size();
		if(index ==0)
		{
			shuffle(Uset);
		}
		if(index == maxindex)
		{
			LOG("[PickState] End of pick");
			level = -1;
			index = 0;
			maxindex = Uset.size();
			return nullptr;
		}
		auto &p = Uset[index];
		LOG("[PickState] Pick State "<<index<<"/"<<maxindex-1<<", U level = "<<p.second <<", state = "<<p.first->latches());
		index++;
		level = p.second;
		return p.first;
		
    }

	State *Checker::pickState(Usequence &U, int& level)
    {
		static std::unordered_set<unsigned> _visited;
		static int record_U=U.size(); // to see if it has changed
		if(record_U>U.size())
		{
			// if it's restarted.
			_visited.clear();
		}
		record_U = U.size();
		for(int i = U.size()-1; i >=0; --i)
		{
			auto frame = U[i];
			for(int j = frame.size() -1; j >=0; --j)
			{ 
				auto state_ptr = frame[j];
				if(state_ptr->is_negp)
				{
					// attention, this function relies on update of solver(add new uc) to work.
					// TODO: look ahead once to avoid invariant check. When start solver can still get a new state, there is no need to check invariant.

					// static State* look_ahead = enumerateStartStates(bi_start_solver);
					State* s = enumerateStartStates(bi_start_solver);
					if(s)
					{
						level = 0;
						// FIXME: whether we should record this.
						// maybe here is just for cex structure.
						updateU(U,s,level,nullptr);
						return s;
					}
					else
						// start states are all tested once.
						continue;
				}
				else if(_visited.find(state_ptr->id) == _visited.end())
				{
					_visited.insert(state_ptr->id);
					level = i;
					return state_ptr;					
				}
			}
		}
		level = -1;
		_visited.clear();
        return nullptr;
    }

	/**
	 * @brief About the principle here, see newpartialsolver.h
	 * 
	 * @param s 
	 * @param prior_state 
	 */
	State* Checker::get_partial_state(Assignment& s, const State * prior_state)
	{		
		Assignment next_assumptions = s;
		Assignment old_inputs(s.begin(),s.begin() + model_->num_inputs());
		
		if(prior_state)
		// it is not start state
		{
			// negate the s' as the cls, put into the solver.
			Assignment cls = negate(prior_state->s());
			bi_partial_solver->new_flag();
			bi_partial_solver->add_clause_with_flag(cls);
			
			// the assumption being t's input and latches. 
			next_assumptions.push_back(bi_partial_solver->get_flag());
			bi_partial_solver->set_assumption(next_assumptions);
			
			// Therefore, it ought to be unsat. If so, we can get the partial state (from the uc)
			bool res = bi_partial_solver->solve_assumption();
			// uc actually stores the partial state.
			s = bi_partial_solver->get_conflict();
			
			if(res || s.empty())
			{
				// all states can reach this state! It means the counter example is found. Sure the initial state can reach this state.
				// set the next state to be the initial state.
				assert(Ub.size() && Ub[0].size());
				State* init = Ub[0][0];
				s = init->s();
			}

			// block this clause.
			int flag = bi_partial_solver->get_flag();
			bi_partial_solver->add_clause(flag);
			
		}
		else
		// for initial states, there is no such "prior state", only "bad"
		{
			next_assumptions.push_back(-bad_);
			bi_partial_solver->set_assumption(next_assumptions);
			bool res = bi_partial_solver->solve_assumption();
			s = bi_partial_solver->get_conflict_no_bad(-bad_);
			// if one-step reachable, should already been found in immediate_satisfiable.
			assert(!s.empty());
		}

		//FIXME: is this `inputs` really useful?
		Assignment inputs, latches;
		for (auto& it:s){
			if (abs(it) <= model_->num_inputs ())
				inputs.push_back (it);
			else
				latches.push_back (it);
		}
		if(inputs.empty()) // use this inputs as the inputs.
		{
			inputs = old_inputs;	
		}
		
		// this state is not a full state.
		State *pstate = new State(inputs,latches);
		clear_defer(pstate);
		return pstate;

	}

    /**
	 * @brief print the evidence. reuse backward_first, which exactly reveals present searching direciton.
	 * @pre counterexmple is met already.
	 * @param out 
	 */
    void Checker::print_evidence() const
    {
		PRINTIF_PRIOR();
		
		cout << "Counter Example is found in "<< (!backward_first ? "forward" : "backward")<< " search"<<endl;

		// Print Backward chain first.
		bool latch_printed = false;
		
		out << "1" << endl;
		out << "b" << 0 << endl;
		if(counter_start_b)
		// backward_chain is not empty
		{
			State* to_print = counter_start_b;
			std::stack<std::string> helper;
			// if this is just a portion of the chain, there may not be last_inputs.
			// if(!to_print->last_inputs().empty())
			// 	helper.push(to_print->last_inputs());
			while(to_print)
			{
				State* next = prior_in_trail_b.find(to_print) != prior_in_trail_b.end() ? prior_in_trail_b.at(to_print) : nullptr;
				if (next)
					helper.push(to_print->inputs());
				else
					helper.push(to_print->latches());
				to_print = next;
			}
			while(!helper.empty())
			{
				out<<helper.top()<<endl;
				helper.pop();
			}
			latch_printed = true;
		}
		// then print forward chain.
		if(counter_start_f)
		{
			State* to_print = counter_start_f;
			if(!latch_printed)
				out<<to_print->latches()<<endl;
			while(to_print)
			{
				State* next = prior_in_trail_f.find(to_print) != prior_in_trail_f.end() ? prior_in_trail_f.at(to_print) : nullptr;
				out<<to_print->inputs()<<endl;
				to_print = next;
			}
		}
		out << "." << endl;
	}

	void Checker::draw_graph()
	{
		dot_out << "digraph SearchTree {" << endl;
		// if the counter example is found, draw the end node
		std::set<int> spliters={spliter.begin(),spliter.end()};
		if(whichCEX())
		{
			dot_out << whichCEX()->id <<" [style=filled, fillcolor=red];"<<endl;
			if(backward_first)
				dot_out<<whichCEX()->id <<" -> NOTP [style=dashed] ;"<<endl;
			else
				dot_out<<"I ->" << whichCEX()->id << "[style=dashed] ;"<<endl;
		}
		for (auto iter:prior_in_trail_b) {
			if(blocked_ids.count(iter.first->id))
				dot_out<< int(iter.first->id) << " [style = dashed];"<<endl;
			if(spliters.count(iter.first->id))
				dot_out<< int(iter.first->id) << " [color=green];"<<endl;
			else
				dot_out<< int(iter.first->id) << " [color=blue];"<<endl;
			if(!iter.second)
				dot_out<<"I ->" << int(iter.first->id) << ";" << endl;
			else
				dot_out << int(iter.second->id ) << " -> " << int(iter.first->id) << ";" << endl;
		}
		for (auto iter:prior_in_trail_f) {
			if(blocked_ids.count(iter.first->id))
				dot_out<< int(iter.first->id) << " [style = dashed];"<<endl;
			if(spliters.count(iter.first->id))
				dot_out<< int(iter.first->id) << " [color=green];"<<endl;
			else
				dot_out<< int(iter.first->id) << " [color=red];"<<endl;
			if(!iter.second)
				dot_out<<int(iter.first->id) << "-> NOTP " << ";" << endl;
			else
				dot_out <<int(iter.first->id)  << " -> " << int(iter.second->id ) << ";" << endl;
		}
		dot_out << "}" << endl;
	}

	void Checker::print_U_shape()
	{
		int total_uc=0;
		cout<<"shape of O array: [";
		for(int i = 0; i< Onp.size();++i)
		{
			cout<<Onp[i].size()<<", ";
			total_uc += Onp[i].size();
		}
		cout<<"]"<<endl;
		int total_states=0;
		cout<<"shape of U array: [";
		for(int i = 0; i< whichU().size();++i)
		{
			total_states+=whichU()[i].size();
			cout<<whichU()[i].size()<<", ";
		}
		cout<<"]"<<endl;
		cout<<"Total states: "<<total_states<<endl;
		cout<<"Total UC: "<<total_uc<<endl;

		cout<<"spliters id array: [";
		for(int spl:spliter)
		{
			cout<<spl<<", ";
		}
		cout<<"]"<<endl;
		cout<<"tried before count array: [";
		for(int cnt:blocked_counter_array)
		{
			cout<<cnt<<", ";
		}
		cout<<"]"<<endl;
		
	}

    void Checker::print_flags(ostream &out)
    {
		out<<endl;
		out << "------ End Printing Flags ------" << endl;
		for(auto p:SO_map)
		{
			out<<"State is: "<<p.first->id<<endl;
			auto& flags = bi_main_solver->flag_of_O;
			if(flags.find(p.second) != flags.end())
			{
				auto& flag_for_this_O = flags[p.second];
				for(int i = 0; i< flag_for_this_O.size();++i)
					out<<i<<":"<<flag_for_this_O[i]<<", ";
			}
			out<<endl;
		}
		out << "------ End Printing Flags ------" << endl<<endl;
    }

    bool Checker::updateU(Usequence &U, State *s, int level, State * prior_state_in_trail)
    {
		while(U.size() <= level)
			U.push_back({});
		
		// Counter Example Issue. 
		// Every time we insert a new state into U sequence, it should be updated.
		{
			whichPrior()[s] = prior_state_in_trail;
		}



		U[level].push_back(s);

		// #ifdef RANDOM_PICK 
			Uset.push_back({s,level});
		// #endif // RANDOM_PICK
		
		return true;
    }



	static vector<Cube> reorderAssum(vector<Cube> inter, const Cube& rres, const Cube& rtmp)
	{
		vector<Cube> pref;
		#if defined(ASS_IRRI)
		pref= inter;
		if(pref.size() == 0)
		{
			pref = {rres,rtmp};
		}
		else
		{
			pref.insert(pref.begin()+1,rres);
			pref.insert(pref.begin()+2,rtmp);
		}
		#ifdef PRINT_ASS
		cerr<<"IRRI:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif
		#elif defined(ASS_IIRR)
		pref = inter;
		if(pref.size() == 0)
		{
			pref = {rres,rtmp};
		}
		else if(pref.size() == 1)
		{
			pref.insert(pref.begin()+1,rres);
			pref.insert(pref.begin()+2,rtmp);
		}
		else
		{
			pref.insert(pref.begin()+2,rres);
			pref.insert(pref.begin()+3,rtmp);
		}
		#ifdef PRINT_ASS
		cerr<<"IIRR:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif
		#elif defined(ASS_IRIR)
		pref = inter;
		if(pref.size() == 0)
		{
			pref = {rres,rtmp};
		}
		else if(pref.size() == 1)
		{
			pref.insert(pref.begin()+1,rres);
			pref.insert(pref.begin()+2,rtmp);
		}
		else
		{
			pref.insert(pref.begin()+1,rres);
			pref.insert(pref.begin()+3,rtmp);
		}
		#ifdef PRINT_ASS
		cerr<<"IRIR:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif
		#elif defined(ASS_RIRI)
		pref = inter;
		if(pref.size() == 0)
		{
			pref = {rres,rtmp};
		}
		else
		{
			pref.insert(pref.begin()+0,rres);
			pref.insert(pref.begin()+2,rtmp);
		}
		#ifdef PRINT_ASS
		cerr<<"RIRI:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif
		#elif defined(ASS_RRII)
		pref = inter;
		pref.insert(pref.begin()+0,rres);
		pref.insert(pref.begin()+1,rtmp);
		#ifdef PRINT_ASS
		cerr<<"RRII:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif
		#elif defined(ASS_RIIR)
		pref = inter;
		if(pref.size() == 0)
		{
			pref = {rres,rtmp};
		}
		else if(pref.size() == 1)
		{
			pref.insert(pref.begin()+0,rres);
			pref.insert(pref.begin()+2,rres);

		}
		else
		{
			pref.insert(pref.begin()+0,rres);
			pref.insert(pref.begin()+3,rtmp);
		}
		#ifdef PRINT_ASS
		cerr<<"RIIR:"<<endl;
		for(auto &cu:pref)
         for(int i:cu)
		cerr<<i<<", ";
		cerr<<endl;
		#endif

		#else
		pref = inter;
		pref.push_back(rres);
		pref.push_back(rtmp);
		#endif


		return pref;
	}

    bool Checker::satAssume(MainSolver *solver, Osequence *O, State *s, int level, const Frame& Otmp)
    {
		++stuck_counter;
		bool res = false;
		if (level == -1)
		{
			// NOTE: in backward CAR, here needs further check.
			if (lastCheck(s, O))
				res = true; // unsafe
			else
				res = false;
			PRINTIF_SIMPLE_SAT();
			return res;    
		}

		// else

		bool forward = !backward_first;
		vector<Cube> inter;

		bool inter_invalidate = false;
		do
		{
			#ifdef INTER
			// intersection:
			const Frame &frame = level+1 < O->size() ? (*O)[level+1] : Otmp;
			int index = 1;

			#ifdef CONVERGENCE
			// FIXME: uniform them!
			int uc_index = frame.size();
			int record_bits = conv_record[level+1];

			// cerr<<endl;
			// cerr<<"record bits of "<< level+1<<" are "<< record_bits<<endl;
			// cerr<<"frame size is "<<frame.size()<<endl;
			#endif

			while(index <= get_inter_cnt()  && frame.size() >= index * 1)
			{
				Cube inter_next;	
				// FIXME: the meaning of inter_cnt is not consistent as to ni > 1
				#ifdef INTER_RES_LONG
					int length = 0;
					
					// 3 : magic number at present
					for(int j = 0; j < INTER_CNT; ++j)
					{
						if(frame.size() < index + j)
							continue;
						const Cube& last_uc = frame[frame.size()-index-j];
						auto res = s->intersect(last_uc);
						if(res.size() <= length)
							continue;
						else
						{
							length = res.size();
							inter_next = res;
						}

					}

				#else // not (INTER_RES_LONG)

					#ifdef INTER_RAND
					// the influence of this is not obvious
					#ifdef RANDSEED
					int seed = RANDSEED;
					#else
					int seed =1;
					#endif
					mt19937 mt_rand(seed);
					const Cube& last_uc = frame[mt_rand()%(frame.size())];
					#elif defined(INTER_LONG) 
					int last_uc_index = frame.size() - index;
					if(longest_uc_index.size() > level+2)
						last_uc_index = longest_uc_index[level+1].second;
					const Cube&last_uc = frame[last_uc_index];

					#elif defined(INTER_SHORT) 
					int last_uc_index = frame.size() - index;
					if(shortest_uc_index.size() > level+2)
						last_uc_index = shortest_uc_index[level+1].second;
					const Cube&last_uc = frame[last_uc_index];
					#elif defined(CONVERGENCE)
					uc_index -= 1;
					// cerr<<"index is "<<index<<", record bit is "<< ((record_bits & (1<<(index-1))) ? 1 : 0 )<<endl;
					uc_index -= (record_bits & (1<<(index-1))) ? 1 : 0;
					if(uc_index<0)
						break;
					// cerr<<"picked uc at "<<uc_index<<endl;

					const Cube& last_uc = frame[uc_index];
					#else // CONVERGENCE
					
					const Cube& last_uc = frame[frame.size()-index * 1];
					#endif // INTER_RAND
					// this preserves the last bit
					inter_next = s->intersect(last_uc);
					
					// FIXME: this is ugly. change it later please.
					if(!inter_next.empty() && !s->imply({last_uc.back()}) )
					{
						inter_invalidate = true;
					}
				#endif // end of (INTER_RES_LONG)

				// otherwise, do not do this!
				#ifdef LAST_FIRST
				if(inter_next.size() >1 && !inter_invalidate)
				{
					// insert the last bit to the front.
					inter_next.insert(inter_next.begin(),inter_next.back());
					inter_next.pop_back();
				}
				#elif defined (INTER_REVERSE)
				if(inter_next.size() >1)
				{
					reverse(inter_next.begin(),inter_next.end());
				}
				#elif defined (INTER_LF_SHUFFLE)
				if(inter_next.size() >1)
				{
					int confl_bit = inter_next.back();
					// insert the last bit to the front.
					if(!inter_invalidate)
						inter_next.pop_back();
					shuffle(inter_next);
					if(!inter_invalidate)
						inter_next.insert(inter_next.begin(),confl_bit);
				}
				#elif defined (INTER_SHUFFLE)
				if(inter_next.size() >1)
				{
					if(!inter_invalidate)
						inter_next.pop_back();
					shuffle(inter_next);
				}
				#else
				if(inter_next.size() >1 && !inter_invalidate)
					inter_next.pop_back();
				#endif
				#ifdef INTER_INVALIDATE_SIMPLE
				if( (!inter_invalidate) || O_level_fresh[level])
				{
					inter.push_back(inter_next);
					inter_invalidate = false;
				}
				#elif defined(INTER_INVALIDATE_HARD)
				if( (!inter_invalidate))
				{
					inter.push_back(inter_next);
					inter_invalidate = false;
				}
				#else
				inter.push_back(inter_next);
				#endif
				index++;
			}
			// complete it
			if(inter.empty()) inter.push_back({}); 
			// cout<<"inter size = "<<inter.size()<<" / "<<INTER_CNT<<endl;
			#endif // INTER
		}
		while(0);


		Cube rres, rtmp;
		do
		{	
			#ifdef ROTATE
			// rotates[i]
				if(!rotate_enabled)
					break;
				Cube &rcu = level +1 < rotates.size() ? rotates[level+1] : rotate;
				if(rcu.empty())
				{
					rcu = s->s();
					// TODO: try this to be inter?
					break;
				}

				// calculate intersection and put the others behind.
				for(int i = 0; i < rcu.size(); ++i)
				{
					// full state
					if(s->size() == model_->num_latches())
					{
						if(s->element(abs(rcu[i])-model_->num_inputs ()-1) == rcu[i])
							rres.push_back(rcu[i]);
						else
							rtmp.push_back(-rcu[i]);
					}
					else
					// TODO: merge with "intersection"
					// a partial state
					{
						int i = 0, j = 0;
						while (i < s->size() && j < rcu.size())
						{
							if (rcu[j] == s->element(i))
							{
								rres.push_back(rcu[j]);
								i++;
								j++;
							}
							else
							{
								rtmp.push_back(s->element(i));
								if (rcu[j] < s->element(i))
									j++;
								else
									i++;
							}
						}
					}

				}
					// inter ++ (s ∩ rcu) ++ (s - rcu) ++ s
			#endif
		} while (0);
			
		Cube score_order;
		do
		{
			#ifdef SCORE
			// use score to order the state
			
			// reference to the dict.
			std::unordered_map<int,int>& dict_ref = level + 1 < score_dicts.size() ? score_dicts[level+1] : score_dict;
			
			// first assign to the original order.
			score_order = s->s();

			#ifdef SCORE_REVERSE 
			// from low to high, for sanity check check.
			std::sort(score_order.begin(),score_order.end(),[&](const int&a, const int&b){
				if(dict_ref[a] < dict_ref[b])
					return true;
				else if(dict_ref[a] > dict_ref[b])
					return false;
				else
				{
					return false;
				}
			});
			#else
			// then sort according to the score.
			// high -> low
			// tie : ignore
			std::sort(score_order.begin(),score_order.end(),[&](const int&a, const int&b){
				if(dict_ref[a] < dict_ref[b])
					return false;
				else if(dict_ref[a] > dict_ref[b])
					return true;
				else
				{
					// tie
					// TODO: add dealing here.

					// at present: do not change it.
					return false;
				}
			});
			#endif
			
			// cerr<<"state: ";
			// for(int i:score_order)
			// cerr<<i<<"("<<dict_ref[i]<<"), ";
			// cerr<<endl;
			
			#endif
		} while (0);

		
		#ifdef SCORE
		#undef ROTATE
		// NOTE: rotate and score are contradictory, because they both contain the whole state.
		vector<Cube> pref = inter;
		pref.push_back(score_order);
		#else
		vector<Cube> pref = reorderAssum(inter,rres,rtmp);
		#endif		
		solver->set_assumption(O,s,level,forward,pref);
		res = solver->solve_with_assumption();
		#ifdef FALLIN_STATS

		if(!res)
		{
			{
			bool cons = true;
			Cube uc = bi_main_solver->get_conflict(!backward_first,cons);

			stats.nInAss++;				
			if(pref.size() >0 && imply(pref[0],uc, false) && level != 0)
			{
					stats.nInAss0_succeed++;
			}
			if( pref.size() >1 && imply(pref[1],uc,false) && level != 0){
				stats.nInAss1_succeed++;
				#ifdef ROTATE
				if(rotate_enabled)
				{
					Cube &rcu = level +1 < rotates.size() ? rotates[level+1] : rotate;
					rcu.clear();
				}
				#endif // ROTATE
			}
			}
		}
		#endif

		#ifdef ROTATE
		if(!res)
		{
			if(rotate_enabled)
			{
				Cube st1 = rres;
				st1.insert(st1.end(),rtmp.begin(),rtmp.end());
				Cube &rcu = level +1 < rotates.size() ? rotates[level+1] : rotate;
				rcu = st1;
			}
		}
		#endif

		#ifdef SCORE
		if(!res)
		{
			std::unordered_map<int,int>& dict_ref = level + 1 < score_dicts.size() ? score_dicts[level+1] : score_dict;
			
		#ifdef SCORE_DECAY
			// multiplicative decay
			if(decayCounter[level + 1] == 0)
			{
				// Magic Number 20.
				decayCounter[level + 1] = 20;
				
				if(decayStep[level+1] == 0)
					decayStep[level+1] = 1000;
				else
					decayStep[level+1]*= 1.01;
				
				// to avoid overflow
				if(decayStep[level+1] > 1000000000)
				{
					auto& dict = score_dicts[level + 1];
					for(auto& pair:dict)
					{
						pair.second >>= 28;
					}
					decayStep[level+1] = 1000;
				}

			}
			decayCounter[level + 1]--;

			// plus bumping
			for(auto &lit : s->s())
			{
				#ifdef SCORE_ABS
				dict_ref[abs(lit)]+=decayStep[level+1];
				#else
				dict_ref[lit]+=decayStep[level+1];
				#endif
			}
		#else
			for(auto &lit : s->s())
			{
				#ifdef SCORE_ABS
				dict_ref[abs(lit)]++;
				#else
				dict_ref[lit]++;
				#endif
			}
		#endif
			// cerr<<"state:";
			// for(int i:s->s())
			// cerr<<i<<", ";
			// cerr<<endl;

			// cerr<<"score:";
			// for(auto i:dict_ref)
			// cerr<<i.first <<":"<<i.second<<", ";
			// cerr<<endl;
			
			// TODO: As to those appear in UC, should we add more scores?
		}

		#endif

		#ifdef CONVERGENCE // suggested by Ofer
		#undef FRESHUC
		// NOTE: should be used together with FRESHUC=OFF
		// <others> <skipped> <previousUC>
		
		// whether to calculate another UC.
		bool trigger = false;
		#ifdef CONV_LOW_THRESH
		// if the level is low, do it!
		if(level < 1 + (O->size() / CONV_LOW_THRESH))
		{
			trigger = true;
		}
		#endif // CONV_LOW

		#ifdef CONV_STUCK_THRESH
		if(stuck_counter > CONV_STUCK_THRESH)
			trigger = true;
		#endif // CONV_STUCK

		// if neither is on, then turn it on always
		#ifndef CONV_LOW_THRESH
			#ifndef CONV_STUCK_THRESH
			trigger = true;
			#endif
		#endif

		// retrieve another bit.
		conv_record[level+1] <<= 1;

		if(!res && trigger)
		{
		// mark as inserted
		conv_record[level+1] += 1;

		clock_high timer_for_calc = high_resolution_clock::now();
		
		// the previous UC. in reverse order of previous assumption.
		auto uc = solver->get_uc();
		model_->shrink_to_latch_vars (uc);

		// update O
		addUCtoSolver(uc,O,level+1, const_cast<Frame&>(Otmp));
		O_level_fresh[level+1]=true;
		O_level_fresh_counter[level+1] = 0;
	
		// last assumption
		auto assumps = solver->get_assumption();
		int flag = assumps.front();

		// next assumption, whose size will be the same as present.
		vector<int> next_ass = {flag};
		next_ass.reserve(assumps.size());

	#ifdef CONV_HEAVY // calculate <skipped>

		// the last lit of previous UC.
		auto splitter = uc.front();

		// find the first occureence of splitter.
		// 1 : The first is the flag of frame.
		int index = 1;
		
		// record what are the <last> + <skipped>, since there are duplicate literals.
		std::unordered_set<int> last_hash = {flag};

		// update the index
		for(;index<assumps.size(); ++index)
		{
			last_hash.insert(assumps[index]);
			if(assumps[index] == splitter)
				break;
		}
	
		// <others>
		for(int i = index + 1; i<assumps.size(); ++i)
		{
			if(last_hash.find(assumps[i]) == last_hash.end())			
				next_ass.push_back(assumps[i]);
		}

		// record previous UC
		std::unordered_set<int> uc_hash = {flag};
		for(auto lit: uc)
			uc_hash.insert(lit);
		
		// <skipped>
		for(int i = 0; i<= index; ++i)
		{
			if(uc_hash.find(assumps[i]) == uc_hash.end())
				next_ass.push_back(assumps[i]);
		}

		// <previous uc>
		for(auto lit: uc)
		{
			next_ass.push_back(lit);
		}

	#else

		std::unordered_set<int> uc_hash;
		for(auto lit: uc)
			uc_hash.insert(lit);
		
		// <skipped + rest part>
		for(int i = 1; i < assumps.size(); ++i)
		{
			if(uc_hash.count(assumps[i]) == 0)
				next_ass.push_back(assumps[i]);
		}
		
		// <last UC>
		for(auto lit : uc)
		{
			next_ass.push_back(lit);
		}

	#endif
		// same, but lighter
		// solver->set_assumption(O,s,level,forward,{next_ass});
		solver->assumption_.clear();
		solver->assumption_.push(solver->SAT_lit(flag));
		for(auto lit:next_ass)
			solver->assumption_.push (solver->SAT_lit (lit));


		stats.conv_calc_acc += duration_cast<milliseconds>(high_resolution_clock::now() - timer_for_calc).count();
		
		timer_for_calc = high_resolution_clock::now();
		
		res = solver->solve_with_assumption();

		auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - timer_for_calc).count();

		stats.conv_second_solve_acc += duration;


		auto next_uc = solver->get_uc();
		model_->shrink_to_latch_vars (next_uc);
		stats.converge_total++;
		
		#ifdef CONV_STAT
		if(imply(next_uc,uc,false))
			stats.converge_implied++;
		#endif

		// cerr<<"assumption:";
		// for(auto lit: assumps)
		// 	cerr<<lit<<", ";
		// cerr<<endl;

		// cerr<<"uc:";
		// for(auto lit: uc)
		// 	cerr<<lit<<", ";
		// cerr<<endl;

		// cerr<<"next ass:";
		// for(auto lit: next_ass)
		// 	cerr<<lit<<", ";
		// cerr<<endl;
		// cerr<<"next ass':";
		// for(auto lit: solver->get_assumption())
		// 	cerr<<lit<<", ";
		// cerr<<endl;

		// cerr<<endl;
		}
		#endif // end CONVERGENCE

		PRINTIF_QUERY();
		PRINTIF_SIMPLE_SAT();
		return res;    
	}


	State* Checker::getModel(MainSolver * solver)
	{
		bool forward = !backward_first;
		State* s = solver->get_state(forward);
		// NOTE: if it is the last state, it will be set last-inputs later.
		clear_defer(s);
		return s;
	}

	State* Checker::getModel(MainSolver * solver, State* prior)
	{
		bool forward = !backward_first;
		if(!forward)
		{
			State* s = solver->get_state(forward);
			// NOTE: if it is the last state, it will be set last-inputs later.
			clear_defer(s);
			return s;
		}
		else
		{
			#ifdef PARTIAL
				Assignment full = solver->get_state_full_assignment(forward);
				return get_partial_state(full,prior);
			#else
				State* s = solver->get_state(forward);
				clear_defer(s);
				return s;
			#endif
		}
	}

    void Checker::clean()
	{
		for(State* duty: clear_duties)
		{
			if(duty)
			{
				delete duty;
				duty = nullptr;
			}
		}
		for(MainSolver* solver: clear_duties_mainsolver)
		{
			if(solver)
			{
				delete solver;
				solver = nullptr;
			}
		}
		for (auto &os : SO_map)
		{
			if (os.second)
			{
				if(os.second==&Onp || os.second == &OI)
					continue;
				delete os.second;
				os.second = nullptr;
			}
		}
	}

	void Checker::clearO(State* s, Osequence *Os)
	{
		assert(s);
		assert(Os);
		Os->clear();
		delete (Os);
		SO_map.erase(s);
	}

	void Checker::blockStateForever(const State* s)
	{
		Cube negstate = negate(s->s());
		bi_main_solver->add_clause(negstate);
		++blocked_count;
		cout<<"blocked_count: "<<blocked_count<<endl;
	}

    void Checker::addUCtoSolver(Cube& uc, Osequence *O, int dst_level, Frame& Otmp)
    {
		if(dst_level < fresh_levels[O])
			fresh_levels[O] = dst_level;
		Frame &frame = (dst_level < int(O->size())) ? (*O)[dst_level] : Otmp;
		// To add \@ cu to \@ frame, there must be
		// 1. \@ cu does not imply any clause in \@ frame
		// 2. if a clause in \@ frame implies \@ cu, replace it by \@cu

		#ifdef FRESH_UC		
		Frame tmp_frame;
		#ifdef INTER_LONG
		if(longest_uc_index.size()<dst_level+1)
			longest_uc_index.resize(dst_level+1);
			int max_len =0, max_index = -1;
		#endif
		#ifdef INTER_SHORT
		if(shortest_uc_index.size()<dst_level+1)
			shortest_uc_index.resize(dst_level+1);
			int min_len =uc.size(), min_index = -1;
		#endif
		for (int i = 0; i < frame.size(); i++)
		{
			if (!imply(frame[i], uc,true))
			{
				tmp_frame.push_back(frame[i]);
				#ifdef INTER_LONG
				if(max_len <= frame[i].size())
				{
					max_len = frame[i].size();
					max_index = tmp_frame.size()-1;
				}
				#endif // INTER_LONG
				#ifdef INTER_SHORT
				if(min_len >= frame[i].size())
				{
					min_len = frame[i].size();
					min_index = tmp_frame.size()-1;
				}
				#endif // INTER_SHORT
			}
		}
		tmp_frame.push_back(uc);

		#ifdef INTER_LONG
		if(max_len <= uc.size())
		{
			max_len = uc.size();
			max_index = tmp_frame.size()-1;
		}
		longest_uc_index[dst_level] = std::pair<int,int>({max_len,max_index});
		#endif // INTER_LONG
		
				
		#ifdef INTER_SHORT
		if(min_len >= uc.size())
		{
			min_len = uc.size();
			min_index = tmp_frame.size()-1;
		}
		shortest_uc_index[dst_level] = std::pair<int,int>({min_len,min_index});
		#endif // INTER_SHORT
		
		frame = tmp_frame;
		#else
		frame.push_back(uc);
		#endif // FRESH_UC

		if(dst_level < O->size())
			bi_main_solver->add_clause_from_cube(uc,dst_level,O,!backward_first);
		else if(dst_level == O->size())
		{
			if(!backward_first)
			{
				// FIXME: test me later.
				// Not always. Only if the start state is ~p.
				bi_start_solver->add_clause_with_flag(uc);
			}
		}

    }

    void Checker::updateO(Osequence *O, int dst, Frame &Otmp, bool& safe_reported)
    {
		Cube uc = bi_main_solver->get_conflict(!backward_first);
		
		if(uc.empty())
		{
			// this state is not reachable?
			//FIXME: fix here. It is not really blocked forever.
			PRINTIF_UNREACHABLE();
			safe_reported = true;
		}
		
		addUCtoSolver(uc,O,dst+1,Otmp);
		O_level_fresh[dst+1]=true;
		O_level_fresh_counter[dst+1] = 0;
		
    }

	bool Checker::initSequence(bool& res)
    {
		State *init = new State(model->init());
		State* negp = new State(true);
		clear_defer(init);
		clear_defer(negp);
		Frame O0; // a frame with only one uc.
		if(immediateCheck(init,bad_,res,O0))
			return true;

		// forward inits.
		if(!backward_first || enable_bi_check)
		{
			// Uf[0] = ~p;

			// NOTE: we do not explicitly construct Uf[0] data strcutre. Because there may be too many states.  We actually need concrete states to get its prime. Therefore, we keep an StartSolver here, and provide a method to enumerate states in Uf[0].
			// construct an abstract state
			Uf = {{negp}};
			prior_in_trail_f[negp] = nullptr;


			// O_I
			// NOTE: O stores the UC, while we actually use negations.
			Frame frame;
			for(auto lit: init->s())
			{
				frame.push_back({-lit});
			}
			OI = {frame};
			SO_map[init]=&OI;
		}
		// backward inits.
		if(backward_first || enable_bi_check)
		{
			// Ub[0] = I;
			updateU(Ub,init,0,nullptr);

			// Ob[0] = uc0.
			// clauses will be added by immediate_satisfible.
			// SAT_assume(init, ~p)
			// uc from init
			// ~p in ~uc
			// use uc to initialize O[0] is suitable. 
			Onp = Osequence({O0});
			SO_map[negp] = &Onp;

		}
		#ifdef ROTATE
		rotates.push_back(init->s());
		#endif

		if (backward_first || enable_bi_check)
			bi_main_solver->add_new_frame(Onp[0], Onp.size() - 1, &Onp, false);
		if (!backward_first || enable_bi_check)
			bi_main_solver->add_new_frame(OI[0], OI.size() - 1, &OI, true);

		O_level_fresh={true};
		O_level_fresh_counter={0};
		O_level_repeat={-2};
		O_level_repeat_counter={0};
		return false;
	}

	//////////////helper functions/////////////////////////////////////////////

	// NOTE: if not updated, it return the same state all the time?
	State *Checker::enumerateStartStates(StartSolver *start_solver)
	{
		// partial state:
		#ifdef PARTIAL
		if(start_solver->solve_with_assumption())
		{
			Assignment ass= start_solver->get_model();
			ass.resize (model_->num_inputs() + model_->num_latches());
			State *partial_res = get_partial_state(ass,nullptr);
			clear_defer(partial_res);
			return partial_res;
		}
		#else
		if (start_solver->solve_with_assumption())
		{
			State *res = start_solver->create_new_state();
			clear_defer(res);
			return res;
		}
		#endif
		return NULL;
	}

	// This is used in sequence initialization
    bool Checker::immediateCheck(State *from, int target, bool &res, Frame &O0)
    {
		// bi_main_solver.
		auto &solver = bi_main_solver; 
		// NOTE: init may not be already set.
		vector<int> latches = from->s();
		
		int last_max = 0;
		do
		{		
			if(solver->solve_with_assumption(latches,target))
			{
				// if sat. already find the cex.
				State *s = solver->get_state(true);// no need to shrink here
				clear_defer(s);
				whichPrior()[s] = from;
				whichCEX() = s;
				res = false;
				return true;
			}
			// NOTE: the last bit in uc is added in.
			Cube cu = solver->get_conflict_no_bad(target); // filter 'bad'
			
			if (cu.empty())
			{
				// this means, ~p itself is bound to be UNSAT. No need to check.
				res = true;
				return true;
			}

			std::set<vector<int>> ucs;
			if(ucs.find(cu)!=ucs.end())
				break;
			// this time's max lit.
			// -2 : because we added a new bit to the end of uc.
			else if(abs(cu[cu.size()-2]) <= last_max)
				break;
			else
			{
				ucs.insert(cu);
				last_max = abs(cu[cu.size()-2]);
				O0.push_back(cu);				
			}
			auto unfresh = [&cu, &last_max](int x) {
        		return abs(x) > last_max;
    		};	
			std::stable_partition(latches.begin(), latches.end(), unfresh);
		}
		while(true);
        return false;
    }

	bool Checker::lastCheck(State* from,  Osequence* O)
	{
		// if(SO_map[State::negp_state] != O) 
		// 	return true;
		bool direction = !backward_first;
		if(direction)
		// in forward CAR, last target state is Init. Initial state is a concrete state, therefore it is bound to be reachable (within 0 steps). Here we only need to update the cex structure.
		{
			whichCEX() = from;
			return true;
		}
		else
		// in backward car, we use uc0 instead of ~P to initialize Ob[0]. This makes it possible to return false, because uc0 is necessary but not essential.  
		{
			// check whether it's in it.
			bool res = bi_main_solver->solve_with_assumption(from->s(),bad_);
			if(res)
			{
				// OK. counter example is found.
				State *s = bi_main_solver->get_state(direction);
				clear_defer(s);
				whichPrior()[s] = from;
				whichCEX() = s;
				return true;
			}
		}
		return false;
	}

	void Checker::print_sat_query(MainSolver *solver, State* s, Osequence *o, int level, bool res, ostream& out)
	{
		static int sat_cnt = 0;
		out<<endl;
		out<<"[SAT] times: "<<++sat_cnt<<"\tlevel="<<level<<endl;
		//<<"\tflag="<<solver->flag_of(o,level)<<endl;
		out<<"SAT solver clause size: "<<solver->size()<<endl;
		out<<"State: ";
		for(int i:s->s())
		out<<i<<", ";
		out<<endl;
		// cout<<"state: \n\tlatches: "<<s->latches()<<"\n\tinputs: "<<s->inputs()<<endl;
		// solver->print_clauses();
		solver->print_assumption(out);
		out<<"res: "<<(res ? "SAT" : "UNSAT")<<endl;
		out<<"-----End of printing-------"<<endl;
	}

	/**
	 * @brief 
	 * @pre s->s() is in abs-increasing order
	 * 
	 * @param s 
	 * @param frame_level 
	 * @param O 
	 * @param Otmp 
	 * @return true 
	 * @return false 
	 */
	bool Checker::blockedIn(const State* s, const int frame_level, Osequence *O, Frame & Otmp)
	{

		assert(frame_level >= 0);
		assert(frame_level <= O->size());
		Frame &frame = (frame_level < O->size()) ? (*O)[frame_level] : Otmp;
		for (auto &uc : frame)
		{
			if(s->imply(uc))
			{
				return true;
			}	
		}
		return false;
	}

	// starting from frame_level, 
	int Checker::minNOTBlocked(const State* s, const int min, const int max, Osequence *O, Frame & Otmp)
	{
		int start = min;
		while(start <= max)
		{
			if(!blockedIn(s,start,O,Otmp))
			{
				break;
			}
			start++;
		}
		return start;
	}
	
    void Checker::removeStateFrom(State *s, Usequence &U)
    {
		int index;
		for(index = 0; index < U.size();++index)
		{
			auto it = std::find(U[index].begin(),U[index].end(), s);
			if(it != U[index].end())
			{
				break;
			}
		}
		auto prior = whichPrior();
		State *p = s;
		while(p && index < U.size())
		{
			auto& subvec=U[index];
			subvec.erase(std::remove(subvec.begin(), subvec.end(), p), subvec.end());
			p = prior[p];
			index++;
		}			
    }
	
	namespace inv
	{
		/**
		 * @brief 
		 * Add the negation of this frame into the solver
		 * @param frame 
		 */
		void InvAddOR(Frame& frame,int level, InvSolver* inv_solver_)
		{
			inv_solver_->add_constraint_or(frame,level);
		}

		/**
		 * @brief 
		 * Add the real states into this solver.
		 * @param frame 
		 */
		void InvAddAND(Frame& frame,int level, InvSolver* inv_solver_)
		{
			inv_solver_->add_constraint_and(frame,level);
		}

		/**
		 * @brief pop the last lit of assumption, and negate it.
		 * 
		 */
		void InvRemoveAND(InvSolver* inv_solver_, int level)
		{
			inv_solver_->release_constraint_and(level);
		}

		bool InvFoundAt(Fsequence &F_, const int frame_level,  InvSolver *inv_solver_, int minimal_update_level_)
		{
			if (frame_level <= minimal_update_level_)
			{
				InvAddOR(F_[frame_level],frame_level, inv_solver_);
				return false;
			}
			InvAddAND(F_[frame_level],frame_level, inv_solver_);
			bool res = !inv_solver_->solve_with_assumption();
			InvRemoveAND(inv_solver_,frame_level);
			InvAddOR(F_[frame_level],frame_level, inv_solver_);
			return res;
		}

		// -1 for not found.
		// >=0 for level
		int InvFound(Model* model_, Fsequence &F_,int& minimal_update_level_, int frame_level)
		{
			if (frame_level == 0)
				return -1;
			int res = -1;
			InvSolver *new_inv_solver_ = new InvSolver(model_);
			for (int i = 0; i < frame_level; i++)
			{
				if (InvFoundAt(F_, i, new_inv_solver_,minimal_update_level_))
				{
					res = i;
					// delete frames after i, and the left F_ is the invariant
					// NOTE: this is temporarily blocked for testing incremental-enumratiing-start-solver.
					// while (F_.size() > i + 1)
					// {
					// 	F_.pop_back();
					// }
					break;
				}
			}
			delete new_inv_solver_;
			return res;
		}
	};
}
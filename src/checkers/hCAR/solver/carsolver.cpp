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


 
#include "carsolver.h"
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
#ifdef MINISAT
	using namespace Minisat;
#else
	using namespace Glucose;
#endif // DEBUG

namespace car
{
 	
	/**
	 * @brief int -> SAT lit
	 * 
	 * @param id 
	 * @return Lit 
	 */
 	Lit CARSolver::SAT_lit (int id) 
 	{
		stats.count_converting_lits_start();
 		assert (id != 0);
        int var = abs(id)-1;
        while (var >= nVars()) newVar();
        auto res = ( (id > 0) ? mkLit(var) : ~mkLit(var) );
		stats.count_converting_lits_end();
		return res;
 	}
 	
	/**
	 * @brief SAT lit -> int
	 * 
	 * @param l 
	 * @return int 
	 */
 	int CARSolver::lit_id (Lit l) const
	{
		stats.count_converting_lits_start();
		if (sign(l)) 
				return -(var(l) + 1);
		else 
				return var(l) + 1;
		stats.count_converting_lits_end();
	}
 	
	/**
	 * @brief Solve with the assumptions in _assumption. 
	 * @note before this, make sure all the assumption lits are put into assumption_.
	 * 
	 * @return true 
	 * @return false 
	 */
 	bool CARSolver::solve_assumption () 
	{
		lbool ret = solveLimited (assumption_);
		if (ret == l_True)
     		return true;
   		else if (ret == l_Undef)
		{
			std::cout<<"Solve Limited failed"<<std::endl;
     		exit (0);
   		}
		return false;
	}
	
	/**
	 * @brief get the assumption in _assumption
	 * 
	 * @return std::vector<int> 
	 */
	std::vector<int> CARSolver::get_assumption() const
	{
		std::vector<int> res;
		res.reserve(assumption_.size());
		for(int i = 0 ; i < assumption_.size();++i)
		{
			res.push_back(lit_id(assumption_[i]));
		}
		return res;
	}

	/**
	 * @brief return the model from SAT solver when it provides SAT
	 * 
	 * @return std::vector<int> : the result from Solver.
	 */
	std::vector<int> CARSolver::get_model () const
	{
		std::vector<int> res;
		res.resize (nVars (), 0);
   		for (int i = 0; i < nVars (); i ++)
   		{
     		if (model[i] == l_True)
       			res[i] = i+1;
     		else if (model[i] == l_False)
       			res[i] = -(i+1);
   		}
   		return res;
	}

	/**
	 * 
	 * @brief return the UC from SAT solver when it provides UNSAT
	 * 
	 * @return std::vector<int> 
	 */
 	std::vector<int> CARSolver::get_uc () const
 	{
 		std::vector<int> reason;
		reason.resize(conflict.size(),0);
		// 
 		for (int k = 0; k < conflict.size(); k++) 
 		{
        	Lit l = conflict[k];
        	reason[k] = -lit_id (l);
    	}
    	return reason;
  	}
		
	/**
	 * @brief return the UC without bad from SAT solver(in particular, from `conflict`) when it provides UNSAT
	 * @todo TODO: use std::transform() may help improve the efficiency by parallel.
	 * @return std::vector<int> 
	 */
	std::vector<int> CARSolver::get_uc_no_bad (int bad) const
 	{
 		std::vector<int> reason;
 		for (int k = 0; k < conflict.size(); k++) 
 		{
        	Lit l = conflict[k];
			int id = -lit_id (l);
			if(id!=bad)
	        	reason.push_back (id);
    	}
    	return reason;
  	}
	 	
	/**
	 * @brief 把cube中的每个元素作为一个clause加入。
	 * 
	 * @param cu 
	 */
 	void CARSolver::add_cube (const std::vector<int>& cu)
 	{
 	    for (int i = 0; i < cu.size (); i ++)
 	        add_clause (cu[i]);
 	}

	/**
	 * @brief 把cube中的每个元素取反，合并为一个clause。表示的是该cube的否定。
	 * 
	 * @param cu 
	 */
	void CARSolver::add_cube_negate (const std::vector<int>&cu)
	{
		vector<int> v = cu;
		for(int& i:v)
			i = -i;
		add_clause (v);
	}

	/**
	 * @brief put the vector into SAT Solver. (as a clause)
	 * 
	 * @param v 
	 */
	void CARSolver::add_clause_internal (const std::vector<int>& v)
 	{
 		vec<Lit> lits(v.size());
		int index = 0;
		for (int id : v)
			lits[index++] = SAT_lit(id);
 		bool res = addClause (lits);
		assert(res && "Warning: Adding clause does not success\n");
 	}

	/**
	 * @brief helper function, print last 3 clauses in the Solver.
	 * 
	 */
	void CARSolver::print_last_3_clauses()
	{
		cout << "Last 3 clauses in SAT solver: \n";
		int cnt = 0;
		for (int i = clauses.size () -1 ; i >=0; i--)
		{
			if(++cnt == 4)
				break;
			Clause& c = ca[clauses[i]];
			std::vector<int> vec;
			for (int j = 0; j < c.size (); j ++)
				vec.push_back(lit_id(c[j]));
			std::sort(vec.begin(),vec.end(),[](int a, int b){return abs(a) < abs(b);});
			for (int j = 0; j < vec.size (); j ++)
				cout<<vec[j]<<" ";
			
			cout << "0 " << endl;
		}
	}

	/**
	 * @brief helper function, print all the clauses in the Solver.
	 * 
	 */
 	void CARSolver::print_clauses(ostream & out)
	{
		out << "clauses in SAT solver: \n";
		for (int i = 0; i < clauses.size (); i ++)
		{
			Clause& c = ca[clauses[i]];
			for (int j = 0; j < c.size (); j ++)
				out << lit_id (c[j]) << " ";
			out << "0 " << endl;
		}
	}
	
	/**
	 * @brief helper function, print the assumption in the Solver.
	 * 
	 */
	void CARSolver::print_assumption (ostream & out)
	{
	    out << "assumptions in SAT solver: \n";
		if (!assumption_.size())
			out<<" Empty ";
	    for (int i = 0; i < assumption_.size (); i ++)
	        out << lit_id (assumption_[i]) << " ";
	    out << endl;
	}
	
}

/****************************************************************************
 * Copyright (c) 2005-2009 Kohei Yoshida
 * 
 * This code is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3 only,
 * as published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License version 3 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with this work.  If not, see
 * <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "numeric/lpsolve.hxx"
#include "numeric/lpmodel.hxx"
#include "numeric/exception.hxx"
#include "numeric/matrix.hxx"
#include "unoglobal.hxx"
#include "tool/global.hxx"
#include "numeric/type.hxx"
#include "lpsolve/lp_lib.h"

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <stdio.h>

using namespace ::scsolver::numeric;
using ::std::vector;

namespace scsolver {

namespace numeric { namespace lp {

class LpSolveImpl
{
public:
	LpSolveImpl() {}
	~LpSolveImpl() throw() {}

    void adjustModel(Model& rModel) const;
	void solve();
	Matrix getSolution() const { return m_mxSolution; }

	void setModel( Model* model ) { m_pModel = model; }
    Model* getModel() const { return m_pModel; }	

private:
	Matrix m_mxSolution;
	Model* m_pModel;
};

void LpSolveImpl::adjustModel(Model& rModel) const
{
    if (rModel.getGoal() == GOAL_TOVALUE)
    {
        // Convert this model to simulate solving to a specific value by 
        // adding an additional constraint.

        Matrix costs = rModel.getCostVector();
        double value = rModel.getSolveToValue();

        size_t costSize = costs.cols(); // cost matrix is a single-row matrix.
        vector<double> v;
        v.reserve(costSize);
        for (size_t i = 0; i < costSize; ++i)
            v.push_back(costs(0, i));
        rModel.addConstraint(v, EQUAL, value);

        // does it matter whether to set this to minimize or maximize?  
        rModel.setGoal(GOAL_MINIMIZE); 
    }
}

void LpSolveImpl::solve()
{
    using ::std::vector;

	Model model(*getModel());
    adjustModel(model);
	size_t nDecVarSize = model.getDecisionVarSize();
	size_t nConstCount = model.getConstraintCount();

#if SCSOLVER_DEBUG	
	printf("decision var (%d)\n", nDecVarSize);
	printf("constraint   (%d)\n", nConstCount);
#endif	

	lprec* lp = make_lp(0, nDecVarSize);
	if ( lp == NULL )
		throw RuntimeError( ascii("Initialization error") );

	for (size_t i = 1; i <= nDecVarSize; ++i)
	{
		if( model.getVarPositive() )
			set_lowbo(lp, static_cast<int>(i), 0.0); // positive variable constraint
		else
			set_unbounded(lp, static_cast<int>(i));
		if ( model.getVarInteger() )
			set_int(lp, static_cast<int>(i), 1);
		else
			set_int(lp, static_cast<int>(i), 0);
	}

	// map constraints
	set_add_rowmode( lp, true );
	vector<double> row( nDecVarSize );
	vector<int>   cols( nDecVarSize );
	try
	{
		for (size_t i = 0; i < nDecVarSize; ++i)
			cols.at(i) = static_cast<int>(i) + 1;

		for (size_t i = 0; i < nConstCount; ++i)
		{
			for (size_t j = 0; j < nDecVarSize; ++j)
				row.at(j) = model.getConstraint(i, j);
			int nEqual = EQ;
			switch ( model.getEquality(i) )
			{
			case GREATER_EQUAL:
				nEqual = GE;
				break;
			case LESS_EQUAL:
				nEqual = LE;
				break;
			case EQUAL:
				nEqual = EQ;
				break;
			}
			add_constraintex( lp, nDecVarSize, &row[0], &cols[0], nEqual,
							  model.getRhsValue(i) );
		}

		set_add_rowmode( lp, false );

		// set objective function
		for (size_t i = 0; i < nDecVarSize; ++i)
		{
#if SCSOLVER_DEBUG
			printf("var %d = %f\n", i+1, model.getCost(i));
#endif
			row.at(i) = model.getCost(i);
		}
		set_obj_fnex( lp, nDecVarSize,  &row[0],  &cols[0] );
	}
	catch ( ::std::out_of_range& e )
	{
		Debug( e.what() );
		delete_lp(lp);
		throw RuntimeError( ascii(e.what()) );
	}

	// set goal
	switch ( model.getGoal() )
	{
	case GOAL_MAXIMIZE:
		set_maxim(lp);
		break;
	case GOAL_MINIMIZE:
		set_minim(lp);
		break;
	default:
		delete_lp(lp);
		throw RuntimeError( ascii("Unknown goal") );
	}

	write_LP(lp, stdout);

#if SCSOLVER_DEBUG	
    set_verbose(lp, IMPORTANT);
#else
    set_verbose(lp, NEUTRAL);
#endif    

	if ( ::solve(lp) == OPTIMAL )
	{
		// solution found

		// variable values
		get_variables(lp, &row[0]);
		Matrix mxSolution( nDecVarSize, 1 );
		for ( size_t i = 0; i < nDecVarSize; ++i )
			mxSolution( i, 0 ) = row[i];
		m_mxSolution.swap( mxSolution );
	}
	else
	{
		// solution not found
		delete_lp(lp);
		throw ModelInfeasible();
	}

	delete_lp(lp);
}

//-----------------------------------------------------------------

LpSolve::LpSolve() : m_pImpl( new LpSolveImpl )
{
}

LpSolve::~LpSolve() throw()
{
}

void LpSolve::solve()
{
	m_pImpl->setModel( getModel() );
	m_pImpl->solve();
	setSolution( m_pImpl->getSolution() );
}



}}}

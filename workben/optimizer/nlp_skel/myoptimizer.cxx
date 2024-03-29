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

#include "myoptimizer.hxx"
#include "numeric/nlpmodel.hxx"
#include "numeric/funcobj.hxx"
#include "numeric/exception.hxx"

#include <stdio.h>
#include <vector>

using ::std::vector;
using ::scsolver::numeric::GoalType;

namespace scsolver {

namespace numeric { 

namespace nlp {

MyOptimizer::MyOptimizer()
{
}

MyOptimizer::~MyOptimizer() throw()
{
}

void MyOptimizer::solve()
{
    fprintf(stdout, "solve: --------------------\n");
    BaseFuncObj* functor = getModel()->getFuncObject();
    fprintf(stdout, "solve: %s\n", functor->getFuncString().c_str());

    // Set initial values for the decision variables.  Calling getModel() 
    // returns the current model instance in order to query for model 
    // properties, such as the upper or lower boundaries of each variables,
    // initial variables values etc.
    vector<double> vars;
    getModel()->getVars(vars);

    // Goal can be either GOAL_MAXIMIZE, GOAL_MAXIMIZE, or GOAL_UNKNOWN.
    GoalType goal = getModel()->getGoal();
    switch (goal)
    {
        case GOAL_MAXIMIZE:
            fprintf(stdout, "Let's maximize!\n");
            break;
        case GOAL_MINIMIZE:
            fprintf(stdout, "Let's minimize!\n");
            break;
        case GOAL_UNKNOWN:
        default:
            throw AssertionWrong();
    }

    for (int i = 0; i < 100; ++i)
    {
        vars[0] += 0.1;
        vars[1] += 0.2;

        // Update variable(s) to the functor, and evaluate the function value.
        // If you don't update a variables, then the old value will be retained.
        functor->setVar(0, vars[0]);
        functor->setVar(1, vars[1]);
        double f = functor->eval();

        fprintf(stdout, "solve: f(%.2f, %.2f) = %.2f\n", vars[0], vars[1], f);
    }

    fprintf(stdout, "solution found\n");
}

} // namespace nlp 
} // namespace numeric
} // namespace scsolver

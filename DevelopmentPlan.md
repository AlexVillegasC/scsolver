# Introduction #

This page summarizes my current development plan for this project.  The old development page is located [here](http://wiki.services.openoffice.org/wiki/Optimization_Solver).

# Linear Optimizer #

There is currently no further plan on linear optimizer development since the [lp\_solve](http://lpsolve.sourceforge.net/) library fulfills this role very nicely.  However, I am personally interested in investing some time into the interior point method, which is believed to be more efficient than the simplex based linear algorithms.

# Non-Linear Optimizer #

## Overview ##

The non-linear optimizer is currently under development.  The plan is to basically work on developing a robust _unconstrained_ non-linear optimizer first, then develop a penalty or barrier method to solve _constrained_ non-linear models by converting them into unconstrained models so that the unconstrained optimizer can be used to reach an optimum solution.  There are several different unconstrained non-linear algorithms to choose from, and my plan is to develop several of them and switch between them based on the characteristics of the model.

Another important aspect of developing a non-linear optimizer is to write a robust line search algorithm since a large part of solving a non-linear model involves a number of line search routines.  Here, there are two well-used line search algorithms under consideration: **quadratic fit** and **bi-section** line search algorithms.  It's probably sufficient to stick with the quadratic fit algorithm with the bi-section as a fall-back in case the quad fit fails.

One of the difficulties on developing a non-linear optimizer is the non-convexity of the models to be solved; unlike a linear model, a local optimum for a non-linear model is not guaranteed to be the global optimum, hence the "solution" reached by an optimizer may not be the best solution but merely one of its several local optima that may be even distance away from the global optimum.  Some non-linear algorithms claim to be better at reaching global optima than others, but my initial plan is to introduce a genetic algorithm to randomize the initial points and perform several optimization runs until a sufficiently optimum "solution" is reached, instead of working on the algorithm itself to make it more capable of reaching global optima.  The disadvantage of this approach is that there is no way of knowing whether the solution reached by the optimizer is truly the global optimum, but that's the limitation I'm willing to accept until a better strategy is found.

## Unconstrained algorithms ##

### Quasi-Newton with BFGS update ###

### Hooke Jeeves ###

## Constrained to unconstrained conversion algorithms ##

### Penalty ###

### Barrier ###

## Initial point randomization ##

### Genetic algorithm ###

We plan to use genetic algorithm to randomize the initial position to raise the odds of reaching the global optimum of a non-linear model.

# Third Party Library Integration #

I write my own algorithm because that is my passion.  However, I would like to also integrate third party non-linear optimizers because I believe in choice.  Providing more choices in optimizers would empower users, and improves the usefulness of scsolver as a general purpose optimization solver front-end.

# OOo Calc Integration #

## UNO API ##

Calc implements the [Solver](http://api.openoffice.org/docs/common/ref/com/sun/star/sheet/Solver.html) service to programatically allow access to Solver.  We need to support this service to be fully compatible with Sun's version so that it can serve as its complete replacement.  This service implements the [XSolver](http://api.openoffice.org/docs/common/ref/com/sun/star/sheet/XSolver.html) interface.

Our eventual goal is to export our own API for linear programming as well as non-linear.

## Progress bar ##

We need to add a progress bar with a cancel button to allow aborting of an on-going optimization run.

# Libraries #

  * [GNU MP Bignum Library (GMP)](http://gmplib.org/)
  * [C-XSC](http://xsc.de/)
  * [Xiaofeng XIE's Research Page](http://www.adaptivebox.net/research/)

# References #
  * [Design and Use of the Microsoft Excel Solver](http://www.utexas.edu/courses/lasdon/design3.htm)
  * [Affine arithmetic and its applications](http://reliablecomputing.eu/) by Ali Baharev.
  * [Interval Methods for Accelerated Global Search in the Microsoft Excel Solver](http://www.solver.com/IntervalSolverRC.pdf)
  * [Lagrangian Global Optimizer (LaGO)](https://projects.coin-or.org/LaGO)
  * [Extensions - OpenOffice.org Developer's Guide](http://wiki.services.openoffice.org/wiki/Documentation/DevGuide/Extensions/Extensions)
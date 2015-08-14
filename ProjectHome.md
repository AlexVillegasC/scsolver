Linear and non-linear optimization solvers designed for OpenOffice.org Calc.

# Introduction #
The goal of this project is to develop a Calc add-on component that solves a constrained linear or non-linear programming model for an optimum solution. An optimum solution in this case is defined as a feasible point that either maximizes or minimizes the objective function while satisfying all given constraints.

Please note that the [Go-oo](http://go-oo.org) variant of OpenOffice.org ships scsolver as the default Solver for Calc.  So if you use Go-oo, you don't need to install scsolver as a separate extension package.

# Core Developer #
Kohei Yoshida

# Credits #
I would like to thank the following individuals for their past contribution to this project.

  * Federico Zenith --- cleaned up and commented the function object implementation code, plus a bug fix.
  * Michael Meeks --- initial OOo integration and general design advice
  * Jody Goldberg --- general advice on optimizer development and overall design
  * Tor Lillqvist --- win32 build fixes
  * Rene Engelhard --- Debian build fixes
  * Laurent Godard --- French translation & UNO API help
  * Kazunari Hirano --- Japanese translation
  * Konstantin Lavrov --- Russian translation & menu integration
  * Rolf Meyer --- German translation
  * Mario Alfredo Renderos --- Spanish translation

# Hacking Tips #
Please read the HackersGuide wiki page for building instructions and code organization of the scsolver project.  If you need additional information or need more clarification on particular topics, feel free to leave a comment in the comment section.
# Introduction #

The goal of this page is to provide a quick documentation on how to get started on hacking the scsolver code, and to give a brief overview of how the source code is organized.

I will assume you are already familiar with the basic use of svn and how to check out the scsolver code from our svn repository.  If not, please read [this page](http://code.google.com/p/scsolver/source/checkout).  I will also (rather selfishly) assume that you are going to develop scsolver on Linux since I am only familiar with how to develop efficiently on that platform.

# Building scsolver #

## Getting what you need ##
scsolver is developed as an external component to OpenOffice.org (or OOo for short), hence developing scsolver does not require ever touching OOo core's code.  All you need to have is an OOo installation and the SDK whose version matches the version of your OOo installation.  _(It is currently tested to work on the 2.4 branch, so it may not even compile against the 3.0 branch at the moment.)_

You also need the lp\_solve library installed on your system.  You can download the latest lp\_solve package either from the [official project site](http://sourceforge.net/projects/lpsolve/), or get it from the [go-oo server](http://download.go-oo.org/SRC680/lp_solve_5.5.0.12_source.tar.gz).

Most Linux distros provide the OOo SDK package that matches the version of the OOo that they provide.  So it's probably most convenient to just install the SDK from their package repository.  In openSUSE, for instance, you can just install two additional packages - **OpenOffice\_org-devel** and **OpenOffice\_org-sdk** - in order to build scsolver.  I believe it is similar with other distros, although the exact package names may vary from distro to distro.

## Building scsolver package ##
### Linux ###
To compile scsolver as an extension package **scsolver.uno.oxt** on Linux, follow these steps:

  1. First, you need to have OOo installed on your system, and the OOo SDK whose version matches the version of your OOo.  Additionally, you need to have downloaded and built the lp\_solve library in a separate directory.  If you have read the previous section, you should have already downloaded the source code of lp\_solve.  The following commands will download and build lp\_solve
```
  wget http://download.go-oo.org/SRC680/lp_solve_5.5.0.12_source.tar.gz
  tar xvf lp_solve_5.5.0.12_source.tar.gz
  cd lp_solve_5.5/lpsolve55
  sh ccc
```
  1. Now, change directory into ext, and run autogen.sh with the locations of OOo installation, OOo SDK and lp\_solve specified
```
  cd ext
  ./autogen.sh \
      --with-ooo=/path/to/ooo/install \
      --with-ooo-sdk=/path/to/ooo/sdk \
      --with-lpsolve=/path/to/lpsolve
```

> Be sure to use an absolute path to specify their location.  This will create a configure script and run it to check for various requirements on building scsolver.
  1. If all goes well i.e. the configure script returns no errors, simply run make to build scsolver.uno.oxt.
```
  make
```
  1. At this point, you can launch OOo Calc and manually install scsolver.uno.oxt in the Extension Manager from the GUI menu , or run
```
  make install
```
> from the command line to install it.

That's all, folks.

### Windows ###
On Windows, the building steps are a little dis-organized since I'm less familiar with Windows development.  There is no automated build process available yet, so the build process on Windows is currently done with a simple batch file with hard-coded paths (located in the `ext` directory).  If you are familiar with automating build process on Windows, please let me know.

### As part of OOo build process (for all platforms) ###
There is even another way to build scsolver which is to use OpenOffice.org's build infrastructure.  Since scsolver was originally developed with the eventual OOo integration in mind, it has everything it needs to be built as an OOo module.  It is in fact still built as an OOo module for the [go-oo](http://go-oo.org) variant of OOo.  So, if you already have an OOo build environment available, then this may be the easiest way to build scsolver.

To build it as part of the go-oo build, refer to the [go-oo developers page](http://go-oo.org/developers/) and build it with `--enable-scsolver` configure option.

# Hacking Code #

## Code Organization Overview ##
The `scsolver` module is structured as follows:

  * `prj` - contains files that are necessary for OO.o build process.
  * `source` - main source directory
    * `inc` - headers
    * `numeric` - numerical code (including optimization/matrix codes)
    * `service` - UNO interface
    * `ui` - UI code (uses UNO's dialog framework - fairly rudimentary)
    * `tool` - utility code used throughout the module.
  * `util` - contains module build scripts for OOo build environment.
  * `workben` - contains files that are not part of the build process.
    * `optimizer`
      * `nlp_skel` - set of skeleton files for independent non-linear optimizer development.
  * `ext` - contains files necessary to build a stand-alone scsolver as a UNO extension.

The backend numerical codes are all found in `source/numeric`, whereas all the UI related codes are found in `source/ui`.

## Wrapper for lp\_solve ##

All code that interfaces with lp\_solve are located in:

  * source/inc/numeric/lpsolve.hxx
  * source/numeric/lpsolve.cxx

Class `LpSolve`, a derived class of `BaseAlgorithm`, is responsible for translating scsolver's model information to lp\_solve, have lp\_solve search for an optimal solution, and translate the result back into scsolver's model so that scsolver can put the result back into spreadsheet's cells.
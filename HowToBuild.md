How to build the project on windows using Visual C++.

## Pre-requisites ##

Make sure you have an up-to-date version of the mercurial repository from google code;
It should contain at least the following directories:
  * include/ : libraries' header files (.h) ;
  * lib/ : libraries statically compiled files (.lib) ;
  * src/ : source code of the project ;
  * dll/ : dynamic libraries (.dll)

## Install and configure Visual C++ ##

  * Follow instructions from winamp developers wiki : paragraph "Creating a project" in http://dev.winamp.com/wiki/Beginner%27s_Basic_Plugin_Guide page.

  * Copy the source code files (from src/) in the project directory and add them to the project in Visual C++.

  * Switch project from Debug to Release mode (Project > Properties > Configurations manager).

  * Visual C++ automatically  adds 2 files "Stdafx.h" and "Stdafx.cpp" in the project, which are however of no use ; remove them and disable the pre-compiled headers functionality (Project > Properties > Configuration properties > C/C++ > Pre-compiled headers) ;

  * For Visual C++ 2008: in Project > Properties > Configuration properties > Linker > Input > Additionnal dependencies, remove "$(NoInherit)".
  * For Visual C++ 2010: in Project > Properties > Configuration properties > Linker > Input > Additionnal dependencies, make sure the option "Inherit default parameters from parent or project" is enabled.

  * Remove file "AssemblyInfo.cpp" from the project.

  * Register the following directories (from include/ directory of the repository) as an include directory in Visual C++ (Project > Properties > Configuration properties > C/C++ > General > Additionnal Include directories).
    1. include\ann
    1. include\curl
    1. include\pthread
    1. include\winampsdk

  * Register the lib/ directory from the repository as a lib directory in Visual C++ (Project > Properties > Configuration properties > Linker > General > Additionnal libraries directories).

  * Link the following files to the project (Project > Properties > Configuration properties > Links edition > Input > Additionnal dependencies): libcurl.lib, ANN.lib, pthreadVCE2.lib, nde.lib.

## Compile project ##

Project should now compile, generating gen\_museek.dll in the Release directory.


## How to install Museek (source version) ##

  * Copy ANN.dll, pthreadVCE2.dll, libcurl.dll and zlibwapi.dll from dll/ directory in repository to Winamp/ directory.

  * Copy gen\_museek.dll in Winamp/Plugins/ directory.
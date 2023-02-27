# Installation

The ovni project is developed in a [private
repository](https://pm.bsc.es/gitlab/rarias/ovni) and you will need to request
access to fetch the source. However, a public released version is located
in GitHub in the following URL:

<https://github.com/bsc-pm/ovni>

To clone the repository use:

	$ git clone https://github.com/bsc-pm/ovni

## Build

To build ovni you would need a C compiler, MPI and cmake version 3.20 or newer.
To compile in build/ and install into `$prefix` use:

	$ mkdir build
	$ cd build
	$ cmake -DCMAKE_INSTALL_PREFIX=$prefix ..
	$ make
	$ make install

## Tests

The tests are executed with `make test`. Keep in mind that to enable runtime
tests you will need to have the Nanos6, nOS-V and NODES libraries too while
configuring the project with cmake.


           ovni - Obtuse but Versatile nOS-V Instrumentation

The ovni instrumentation project is composed of a runtime library (libovni.so),
which generates a fast binary trace, and post-processing tools such as the
emulator (emu), which transform the binary trace to the PRV format, suitable to
be loaded in Paraver.

The libovni.so library is licensed under MIT, while the rest of tools are GPLv3
unless otherwise stated.

For more information, take a look at the doc/ directory.

To build ovni you would need a C compiler, MPI and cmake version 3.10 or newer.
To compile in build/ and install into $prefix use:

  % cmake -DCMAKE_INSTALL_PREFIX:PATH=$prefix -S . -B build
  % cd build
  % make
  % make install

See cmake(1) and cmake-env-variables(7) to see more information about
the variables affecting the generation and build process.

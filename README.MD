mC reference AST/parser implementation
=====================================

Setup:
- set correct c++ compiler path in Makefile (requires c++14 support)
  (the path is set to a scratch location accessible from zid-gpl / lab rooms)
- if not using the default system gcc, export its library path
  (e.g. export LD_LIBRARY_PATH=/scratch/c703/c703432/gcc-5.3.0-install/lib64)
- make
- test by running ./mC test.mC

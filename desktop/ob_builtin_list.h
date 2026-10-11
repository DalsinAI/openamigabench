/* The programs built into OpenBench (ob_builtin.h): one BUILTIN(name, entry, stack) a line, each inside the
 * #ifdef that desktop/build.sh defines when it compiles that program in. name is the program's file name
 * (what its path ends in); entry its main() renamed. Keep the lines in alphabetical order. */
#ifdef OB_HAVE_EXAMPLE
BUILTIN("Example", example_main, 16384)
#endif

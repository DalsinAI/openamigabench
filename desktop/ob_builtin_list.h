/* The programs built into OpenBench (ob_builtin.h): one BUILTIN(name, entry, stack) a line, each inside the
 * #ifdef that desktop/build.sh defines when it compiles that program in. name is the program's file name
 * (what its path ends in); entry its main() renamed. Keep the lines in alphabetical order. The editors OpenPrefs
 * has as pages left with OpenBench 0.1.32: their old names open OpenPrefs at their page (openbench.c). */
#ifdef OB_HAVE_EXAMPLE
BUILTIN("Example", example_main, 16384)
#endif
#ifdef OB_HAVE_OPENTATA
BUILTIN("OpenTata", opentata_main, 32768)           /* Tata's window (openamigatata); Tata's checks are openbench.c's */
#endif
#ifdef OB_HAVE_OPENUP_SETUP
BUILTIN("OpenUp-Setup", openup_setup_main, 16384)
#endif

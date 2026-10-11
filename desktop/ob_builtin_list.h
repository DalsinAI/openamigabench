/* The programs built into OpenBench (ob_builtin.h): one BUILTIN(name, entry, stack) a line, each inside the
 * #ifdef that desktop/build.sh defines when it compiles that program in. name is the program's file name
 * (what its path ends in); entry its main() renamed. Keep the lines in alphabetical order. */
#ifdef OB_HAVE_EXAMPLE
BUILTIN("Example", example_main, 16384)
#endif
#ifdef OB_HAVE_LOOK
BUILTIN("Look", look_main, 16384)
#endif
#ifdef OB_HAVE_MENUS
BUILTIN("Menus", menusprefs_main, 16384)
#endif
#ifdef OB_HAVE_OPENDATETIME
BUILTIN("OpenDateTime", opendatetime_main, 32768)
#endif
#ifdef OB_HAVE_OPENDOCK
BUILTIN("OpenDock", dockprefs_main, 16384)          /* the dock's editor, SYS:Prefs/OpenDock (the dock is opendock_main) */
#endif
#ifdef OB_HAVE_OPENGAMEPADS
BUILTIN("OpenGamepads", opengamepads_main, 32768)
#endif
#ifdef OB_HAVE_OPENINPUT
BUILTIN("OpenInput", openinput_main, 32768)
#endif
#ifdef OB_HAVE_OPENSERIAL
BUILTIN("OpenSerial", openserial_main, 16384)
#endif
#ifdef OB_HAVE_OPENSOUND
BUILTIN("OpenSound", opensound_main, 32768)
#endif
#ifdef OB_HAVE_OPENTATA
BUILTIN("OpenTata", opentata_main, 32768)           /* Tata's window (openamigatata); Tata's checks are openbench.c's */
#endif
#ifdef OB_HAVE_OPENTYPES
BUILTIN("OpenTypes", opentypes_main, 16384)
#endif
#ifdef OB_HAVE_OPENUP_SETUP
BUILTIN("OpenUp-Setup", openup_setup_main, 16384)
#endif
#ifdef OB_HAVE_TITLEBAR
BUILTIN("TitleBar", titlebar_main, 16384)
#endif
#ifdef OB_HAVE_WINDOWS
BUILTIN("Windows", windows_main, 16384)
#endif

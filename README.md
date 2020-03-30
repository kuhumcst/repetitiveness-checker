# repetitiveness-checker
Finds repeated sequences of two or more tokens

Sequences are weighted and sorted. The less likely according to word frequencies, the higher up in the resulting list.

This tool often produces some quite long phrases that reflect some of the gist of the input text.

The repetitiveness checker was conceived as a tool to help human translators decide whether to use a translation memory system or not. If a lot of repetitiveness was detected, it was probably worthwhile to put repeated phrases in the translation memory.

## Installing the repetitiveness checker on your computer

The repetitiveness checker depends on another repository. To clone all necessary repositories and build the repetitiveness checker, look at makerepver.bash in the doc folder. If you are in Linux, you can probably just run this script file.

The Makefile in the src directory creates the executable in the repetitiveness-checker folder.

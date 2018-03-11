PawnPlus v0.1.1
==========

_PawnPlus_ extends the possibilities of the Pawn scripting language with new constructs, data types, and programming techniques.

With this plugin, you can use techniques like asynchronous programming, task-based programming, lightweight form of delegates, and dynamic data objects to make programming in Pawn easier, simpler, and more efficient.

## Documentation
See the [wiki](//github.com/IllidanS4/PawnPlus/wiki) for documentation and tutorials on how to use this plugin.

## Installation
Download the latest [release](//github.com/IllidanS4/PawnPlus/releases/latest) for your platform to the "plugins" directory and add "PawnPlus" (or "PawnPlus.so" on Linux) to the `plugins` line in server.cfg.

Include [PawnPlus.inc](pawno/include/PawnPlus.inc) in your Pawn program and you are done. If you don't want to use aliases for native functions, define `PP_NO_ALIASES` before the inclusion.

## Building
Use Visual Studio to build the project on Windows, or `make` on Linux.

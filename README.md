PawnPlus v0.2
==========

_PawnPlus_ extends the possibilities of the Pawn scripting language with new constructs, data types, and programming techniques.

With this plugin, you can use techniques like asynchronous programming, task-based programming, lightweight form of delegates, and dynamic data objects to make programming in Pawn easier, simpler, and more efficient.

## Documentation
See the [wiki](//github.com/IllidanS4/PawnPlus/wiki) for documentation and tutorials on how to use this plugin.

## Installation
Download the latest [release](//github.com/IllidanS4/PawnPlus/releases/latest) for your platform to the "plugins" directory and add "PawnPlus" (or "PawnPlus.so" on Linux) to the `plugins` line in server.cfg.

Include [PawnPlus.inc](pawno/include/PawnPlus.inc) in your Pawn program and you are done.

## Configuration
If you don't want to use aliases for native functions, define `PP_NO_ALIASES` before the inclusion. By default, the `@@` alias to `str_val` is disabled due to conflicts with other scripts, but you can enable it with `PP_MORE_ALIASES`.

To disable automatic calling of `str_val` in concatenations and assignments, define `PP_NO_AUTO_STRINGS`.

## Building
Use Visual Studio to build the project on Windows, or `make` on Linux.

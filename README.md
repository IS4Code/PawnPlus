PawnPlus v0.9
==========

_PawnPlus_ extends the possibilities of the Pawn scripting language with new constructs, data types, and programming techniques.

With this plugin, you can use techniques like asynchronous programming, task-based programming, lightweight form of delegates, and dynamic data objects to make programming in Pawn easier, simpler, and more efficient.

## Documentation
See the [wiki](//github.com/IllidanS4/PawnPlus/wiki) for documentation and tutorials on how to use this plugin.

## Installation
Download the latest [release](//github.com/IllidanS4/PawnPlus/releases/latest) for your platform to the "plugins" directory and add "PawnPlus" (or "PawnPlus.so" on Linux) to the `plugins` line in server.cfg.

Include [PawnPlus.inc](pawno/include/PawnPlus.inc) in your Pawn program and you are done.

## Configuration
This plugin can optionally add a number of syntax features to Pawn like additional statements or operators. These are not available by default due to conflicts with other libraries, but you can use them all if you define `PP_SYNTAX`, or selectively via other definitions. If you are writing a library, it is recommended not to use any configuration definitions.

## Building
Use Visual Studio to build the project on Windows, or `make` on Linux.

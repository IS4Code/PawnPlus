PawnPlus v1.2
==========

_PawnPlus_ extends the possibilities of the Pawn scripting language with new constructs, data types, and programming techniques.

With this plugin, you can use techniques like asynchronous task-based programming, reflection, hooking, and dynamic structures to make programming in Pawn easier, simpler, and more efficient.

_PawnPlus_ introduces dynamically sized strings, lists, linked lists, and maps. Collections can contain any number of elements of any type (cells or arrays) and any tag. They can be iterated using versatile iterator objects. Tasks can be used to execute code based on a specific event, without creating additional callbacks and without blocking the program.

Several new object types use the garbage collector present in the plugin, removing the need to explicitly delete memory in most cases.

## Documentation
See the [wiki](//github.com/IllidanS4/PawnPlus/wiki) for documentation and tutorials on how to use this plugin.

## Installation
Download the latest [release](//github.com/IllidanS4/PawnPlus/releases/latest) for your platform to the "plugins" directory and add "PawnPlus" (or "PawnPlus.so" on Linux) to the `plugins` line in server.cfg.

Include [PawnPlus.inc](pawno/include/PawnPlus.inc) in your Pawn program and you are done.

## Configuration
This plugin can optionally add a number of syntax features to Pawn like additional statements or operators. These are not available by default due to conflicts with other libraries, but you can use them all if you define `PP_SYNTAX`, or selectively via other definitions. If you are writing a library, it is recommended not to use any configuration definitions.

## Building
Use Visual Studio to build the project on Windows, or `make` or `make static` on Linux. Requires GCC >= 4.9.

## Credits
* [Zeex](//github.com/Zeex) for creating [subhook](//github.com/Zeex/subhook) without which this wouldn't be possible.
* [Y_Less](//github.com/Y-Less/) for help with macros for generic functions.
* [TommyB](//github.com/TommyB123) for thorough testing and bug-finding.
* [Southclaws](//github.com/Southclaws/) and [AGraber](//github.com/AGraber) for minor contributions.

Thanks to all _PawnPlus_ users for your support! 
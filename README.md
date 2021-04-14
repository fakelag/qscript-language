# QScript Language

QScript is an interpreted programming language written in C++ with a syntax like JavaScript & Objective-C. The project can be compiled on windows with [Visual Studio](https://visualstudio.microsoft.com/) and on OSX via [gcc](https://gcc.gnu.org/).

## Installation (Microsoft Windows)
- Install Visual Studio 2019 (any with v142 platform toolset will work)
- Open `qscript-language.sln`
- You can now compile and run any of the projects for x86/x64 release + debug

## Installation (OSX)
- Install gcc (g++)
- Run
```bash
cd OSX

sh ./compile_pch.sh 		# Re-run if Includes/QLibPCH.h gets changed
sh ./compile_cli.sh 		# Compiles CLI project

./Lib/CLI.o --repl 			# Start CLI in REPL mode
```

## Basic language features

![example 1](https://github.com/fakelag/property-bot/blob/master/media/01.gif)

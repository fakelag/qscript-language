# QScript Language

## Overview

QScript is an interpreted programming language written in C++. The project can be compiled on windows with [Visual Studio](https://visualstudio.microsoft.com/) and on OSX via [gcc](https://gcc.gnu.org/).

## Syntax & language features

The syntax of QScript could be described as a mix of JavaScript/TypeScript & Objective-C. It offers optional typing similar to TypeScript and borrows some grammar -- such as its function call syntax -- from Objective-C. Feature-wise QScript supports everything from basic variables, loops, control-flow statements, functions and arrays to more advanced features such as tables, closures, automatic garbage collection, optional types and langsrv integrations. Unlike some of my other programming language projects, this is a more or less general purpose programming language that doesn't solve for a specific use-case.

![Example 1 - Hello world](https://github.com/fakelag/qscript-language/blob/master/media/01.gif)

![Example 2 - Array sum](https://github.com/fakelag/qscript-language/blob/master/media/02.gif)

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

## Tests

Repository contains unit and ETE tests in `Tests/`. Most tests are end-to-end tests for

1. modeling real-world use-cases as closely as possible
2. testing all components of the language with a single set of tests
3. depending as little as possible on implementation details and focus on behavior

![Running tests](https://github.com/fakelag/qscript-language/blob/master/media/03.gif)

## Repl

CLI contains repl mode which can be used with the `--repl` flag

![Repl mode](https://github.com/fakelag/qscript-language/blob/master/media/04.gif)

## Typing system

QScript contains optional compile-time types -- you can choose to use types or ignore them entirely

![Optional types](https://github.com/fakelag/qscript-language/blob/master/media/05.gif)

You can check also types with `--typer` CLI flag

```bash
Typer >var x = 1;
unknown -> none
Typer >const y = 1;
num -> none
Typer >const f = () -> auto { return "hello"; }
function -> string
Typer >
```

## Language server (& vscode extension)

JSON-based language server allows for easy integration of 3rd party software. For example, a vscode language extension ([link to full repo here](https://github.com/fakelag/qscript-lsp))

![vscode extension](https://github.com/fakelag/qscript-language/blob/master/media/07.gif)

## Debugger

Compiling the library with QVM_DEBUG preprocessor define will yield an interactive debugger for programs loaded in the VM

![CLI Debugger](https://github.com/fakelag/qscript-language/blob/master/media/06.gif)

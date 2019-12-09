g++ -I ../Includes/ ../Tests/Tests.cpp ../Tests/TestLexer.cpp ../Tests/TestCompiler.cpp ../Tests/TestInterpreter.cpp ./Lib/QScript_nodbg.a -Wc++11-extensions -std=c++11 -o ./Lib/Tests.o -D _OSX -g

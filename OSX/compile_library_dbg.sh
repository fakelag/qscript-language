mkdir ./Lib/Tmp

cd ./Lib/Tmp/
g++ -include-pch ../../../Includes/QLibPCH.h.gch ../../../Library/Compiler/*.cpp ../../../Library/Compiler/AST/*.cpp ../../../Library/Runtime/*.cpp ../../../Library/Common/*.cpp ../../../Library/STL/*.cpp -I ../../../Includes/ -Wc++11-extensions -std=c++11 -c -D _OSX -D QVM_DEBUG -D _DEBUG -g
ar rvs ../QScript.a *.o

cd ../..

rm -rf ./Lib/Tmp

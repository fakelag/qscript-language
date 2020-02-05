mkdir ./Lib/Tmp

cd ./Lib/Tmp/
g++ -include-pch ../../../Includes/QLibPCH.h.gch ../../../Library/Compiler/*.cpp ../../../Library/Runtime/*.cpp ../../../Library/Common/*.cpp ../../../Library/STL/*.cpp -I ../../../Includes/ -Wc++11-extensions -std=c++11 -c -D _OSX -D QVM_AGGRESSIVE_GC -D NDEBUG -O3
ar rvs ../QScript.a *.o

cd ../..

rm -rf ./Lib/Tmp

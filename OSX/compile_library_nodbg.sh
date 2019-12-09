mkdir ./Lib/Tmp

cd ./Lib/Tmp/
g++ -include-pch ../../../Includes/QLibPCH.h.gch ../../../Library/Compiler/*.cpp ../../../Library/Runtime/*.cpp ../../../Library/Common/*.cpp -I ../../../Includes/ -Wc++11-extensions -std=c++11 -c -D _OSX -g
ar rvs ../QScript_nodbg.a *.o

cd ../..

rm -rf ./Lib/Tmp

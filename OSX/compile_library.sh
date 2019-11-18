mkdir ./Lib/Tmp

cd ./Lib/Tmp/
g++ ../../../Library/Compiler/*.cpp ../../../Library/Runtime/*.cpp -I ../../../Includes/ -Wc++11-extensions -std=c++11 -c -D _OSX
ar rvs ../QScript.a *.o

cd ../..

rm -rf ./Lib/Tmp

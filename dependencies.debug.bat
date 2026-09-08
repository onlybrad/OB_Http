REM build libcurl debug (dynamic)
mkdir vendors\libcurl\build\debug
cd vendors\libcurl\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DUSE_SCHANNEL=ON -DUSE_WINDOWS_SSPI=ON
cmake --build . --config Debug -- -j%NUMBER_OF_PROCESSORS%
copy lib\libcurl-d.dll ..\..\..\..\
cd ..\..\..\..

REM build libtidy release (dynamic)
mkdir vendors\libtidy\build\debug
cd vendors\libtidy\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Release -- -j%NUMBER_OF_PROCESSORS%
rename libtidy.dll libtidy-d.dll
copy libtidy-d.dll ..\..\..\..\
cd ..\..\..\..

REM build CJSON release (static)
cd vendors\CJSON
make static
copy CJSON.a ..\..
cd ..\..
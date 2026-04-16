REM build libcurl debug (dynamic)
mkdir vendors\libcurl\build\debug
cd vendors\libcurl\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DUSE_SCHANNEL=ON -DUSE_WINDOWS_SSPI=ON
cmake --build . --config Debug -- -j%NUMBER_OF_PROCESSORS%
copy lib\libcurl.dll ..\..\..\..\
cd ..\..\..\..

REM build CJSON release (static)
cd vendors\CJSON
make static
copy CJSON.a ..\..
cd ..\..
REM build libcurl (dynamic)
mkdir vendors\libcurl\build\debug
cd vendors\libcurl\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON -DUSE_SCHANNEL=ON -DUSE_WINDOWS_SSPI=ON
cmake --build . --config Debug
copy lib\libcurl-d.dll ..\..\..\..\
cd ..\..\..\..

REM build CJSON (static)
cd vendors\CJSON
make static
copy CJSON.a ..\..
cd ..\..
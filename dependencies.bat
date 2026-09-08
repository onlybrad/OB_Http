REM build libcurl release (dynamic)
mkdir vendors\libcurl\build\debug
cd vendors\libcurl\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_SCHANNEL=ON -DUSE_WINDOWS_SSPI=ON
cmake --build . --config Release -- -j%NUMBER_OF_PROCESSORS%
copy lib\libcurl.dll ..\..\..\..\
cd ..\..\..\..

REM build libtidy release (dynamic)
mkdir vendors\libtidy\build\debug
cd vendors\libtidy\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -- -j%NUMBER_OF_PROCESSORS%
copy libtidy.dll ..\..\..\..\
cd ..\..\..\..

REM build CJSON release (static)
cd vendors\CJSON
make static
copy CJSON.a ..\..
cd ..\..
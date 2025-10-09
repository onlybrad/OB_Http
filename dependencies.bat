mkdir vendors\libcurl\build\debug
cd vendors\libcurl\build\debug
cmake ..\.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DENABLE_DEBUG=ON -DUSE_SCHANNEL=ON -DUSE_WINDOWS_SSPI=ON
cmake --build . --config Debug
cp lib\libcurl-d.dll ..\..\..\..\
cd ..\..\..\..
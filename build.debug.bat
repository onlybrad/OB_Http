call dependencies.debug.bat

gcc -std=c99 -g -c src/*.c -Ivendors\libcurl\include -Ivendors\CJSON -Wall -Wextra -pedantic -Wconversion -Wstrict-overflow=5 -Wshadow -Wunused-macros -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wdangling-else -Wlogical-op -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Winline

ar rcs obhttp-d.a
for %%f in (*.o) do (
    ar r obhttp-d.a %%f
)

del /Q *.o 2>nul
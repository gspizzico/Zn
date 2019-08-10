#include <iostream>
#include <string>
#include <windows.h>

int main()
{
    std::string string = "Hello World!\n";
    std::cout << string;
    OutputDebugStringA(string.c_str());
    return 0;
}
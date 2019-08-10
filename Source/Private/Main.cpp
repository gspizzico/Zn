#include <iostream>
#include <string>
#include <windows.h>

#include "Core/Log/Log.h"
#include "Core/Name.h"
#include "Core/HAL/PlatformTypes.h"

int main()
{   
    //Zn::Log::DefineLogCategory(Zn::Name("Test"), 5);
    for (int32 Index = 0; Index < 1000; Index++)
    {
        auto vn = Zn::Name(std::to_string(Index));
    }
    return 0;
}
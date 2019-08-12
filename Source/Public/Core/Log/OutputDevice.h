#pragma once

namespace Zn
{
    class IOutputDevice abstract
    {
    public:
        virtual void OutputMessage(const char* message) = 0;
    };
}

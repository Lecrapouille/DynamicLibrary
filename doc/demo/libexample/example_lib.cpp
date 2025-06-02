//! ============================================================================
//! \file example_lib.cpp
//! \brief Normal library that can be unloaded
//! ============================================================================

#include <iostream>

extern "C"
{
    int add(int a, int b)
    {
        return a + b;
    }

    int multiply(int a, int b)
    {
        return a * b;
    }

    void print_message(const char* msg)
    {
        std::cout << "Library says: " << msg << std::endl;
    }

    const char* get_version()
    {
        return "1.0.0";
    }
}
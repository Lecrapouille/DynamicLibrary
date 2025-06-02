//! ============================================================================
//! \file good_lib.cpp
//! \brief Well-designed library for reloading
//! ============================================================================

#include <iostream>

// Local variables to the function - no problem
extern "C"
{
    int safe_add(int a, int b)
    {
        return a + b;
    }

    void safe_function()
    {
        // Local variables only
        int local_var = 10;
        std::cout << "Safe function called, local_var = " << local_var
                  << std::endl;
    }

    // Function with explicit cleanup
    void* create_resource()
    {
        return new int(123);
    }

    void cleanup_resource(void* ptr)
    {
        delete static_cast<int*>(ptr);
    }
}

// Balanced constructor and destructor
__attribute__((constructor)) void good_lib_init()
{
    std::cout << "Good library initialized" << std::endl;
}

__attribute__((destructor)) void good_lib_cleanup()
{
    std::cout << "Good library cleaned up" << std::endl;
}
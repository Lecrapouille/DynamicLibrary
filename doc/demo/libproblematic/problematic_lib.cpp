//! ============================================================================
//! \file problematic_lib.cpp
//! \brief Library with problems unloading
//! ============================================================================

#include <iostream>
#include <memory>

// Static variables that prevent proper unloading
static std::unique_ptr<int> global_ptr;
static bool initialized = false;

// Global constructor that runs at loading
__attribute__((constructor)) void library_init()
{
    std::cout << "Problematic library constructor called" << std::endl;
    global_ptr = std::make_unique<int>(42);
    initialized = true;

    // Simulation of a thread or resource that is not cleaned up
    static thread_local int tls_var = 100;
    tls_var++;
}

// No corresponding destructor - problematic!

extern "C"
{
    int problematic_function(int x)
    {
        if (!initialized)
        {
            return -1;
        }
        return x + *global_ptr;
    }

    void create_persistent_resource()
    {
        // Creation of a resource that will never be released
        static int* leak = new int(999);
        std::cout << "Created persistent resource: " << *leak << std::endl;
    }
}

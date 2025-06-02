//! ============================================================================
//! \file example.cpp
//! \brief Example of using the dynamic library
//! ============================================================================

#include "DynamicLibrary/DynamicLibrary.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#    define LIB_EXTENSION ".dll"
#elif defined(__APPLE__)
#    define LIB_EXTENSION ".dylib"
#elif defined(__linux__)
#    define LIB_EXTENSION ".so"
#else
#    error "Unsupported platform"
#endif

// Example of function that we expect to find in the library
typedef int (*AddFunction)(int, int);
typedef void (*PrintFunction)(const char*);

//-----------------------------------------------------------------------------
void example_basic_usage()
{
    std::cout << "\033[32m=== Basic usage example ===\033[0m" << std::endl;

    try
    {
        // Loading a library with automatic reloading
        dl::DynamicLibrary lib("./libexample" LIB_EXTENSION,
                               dl::AutoReload::Enabled);

        // Retrieving a function
        auto add = lib.getSymbol<AddFunction>("add");
        std::cout << "5 + 3 = " << add(5, 3) << std::endl;

        // Using with std::function
        auto print = lib.getFunction<void(const char*)>("print_message");
        print("Hello from dynamic library!");

        // Checking the reloadability
        if (lib.canReload())
        {
            std::cout
                << "\033[32mLibrary can be safely unloaded and reloaded\033[0m"
                << std::endl;
        }
        else
        {
            std::cout << "\033[31mWarning: Library cannot be unloaded\033[0m"
                      << std::endl;
        }
    }
    catch (const dl::DynamicLibraryException& e)
    {
        std::cerr << "\033[31mError: " << e.what() << "\033[0m" << std::endl;
    }
}

//-----------------------------------------------------------------------------
void example_hot_reload()
{
    std::cout << "\033[32m=== Hot reload example ===\033[0m" << std::endl;

    try
    {
        dl::DynamicLibrary lib("./libexample" LIB_EXTENSION,
                               dl::AutoReload::Enabled);

        auto add = lib.getSymbol<AddFunction>("add");
        std::cout << "Initial: 10 + 20 = " << add(10, 20) << std::endl;

        // Simulation of an application loop that checks for updates
        for (int i = 0; i < 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Update file timestamp to trigger reload
            std::filesystem::path libPath("./libexample" LIB_EXTENSION);
            std::filesystem::last_write_time(
                libPath, std::filesystem::file_time_type::clock::now());

            if (lib.checkForUpdates())
            {
                std::cout
                    << "\033[32mLibrary updated detected, reloading...\033[0m"
                    << std::endl;
                lib.reload();

                // Retrieve the symbol again after reloading
                add = lib.getSymbol<AddFunction>("add");
                std::cout << "After reload: 10 + 20 = " << add(10, 20)
                          << std::endl;
            }
            else
            {
                std::cout << "\033[32mNo updates detected\033[0m" << std::endl;
            }
        }
    }
    catch (const dl::DynamicLibraryException& e)
    {
        std::cerr << "\033[31mError: " << e.what() << "\033[0m" << std::endl;
    }
}

//-----------------------------------------------------------------------------
void example_manager()
{
    std::cout << "\033[32m=== Example with library manager ===\033[0m"
              << std::endl;

    dl::DynamicLibraryManager manager;

    try
    {
        // Loading multiple libraries
        auto mathLib = manager.loadLibrary(
            "math", "./libexample" LIB_EXTENSION, dl::AutoReload::Enabled);
        auto utilsLib = manager.loadLibrary(
            "utils", "./libgood" LIB_EXTENSION, dl::AutoReload::Enabled);

        // Using the libraries
        if (mathLib)
        {
            auto add = mathLib->getSymbol<int (*)(int, int)>("add");
            std::cout << "7 + 6 = " << add(7, 6) << std::endl;
        }

        if (utilsLib)
        {
            auto safe_function =
                utilsLib->getSymbol<void (*)()>("safe_function");
            safe_function();
        }

        // Checking for updates for all libraries
        manager.checkAllForUpdates();
    }
    catch (const dl::DynamicLibraryException& e)
    {
        std::cerr << "\033[31mError: " << e.what() << "\033[0m" << std::endl;
    }
}

//-----------------------------------------------------------------------------
void example_error_handling()
{
    std::cout << "\033[32m=== Example of error handling ===\033[0m"
              << std::endl;

    // Test with a file that does not exist
    try
    {
        dl::DynamicLibrary lib("./nonexistent" LIB_EXTENSION,
                               dl::AutoReload::Enabled);
    }
    catch (const dl::DynamicLibraryException& e)
    {
        std::cout << "\033[32mExpected error: " << e.what() << "\033[0m"
                  << std::endl;
    }

    // Test with a symbol that does not exist
    try
    {
        dl::DynamicLibrary lib("./libexample" LIB_EXTENSION,
                               dl::AutoReload::Enabled);
        auto func = lib.getSymbol<void (*)()>("nonexistent_function");
        if (!func)
        {
            std::cout << "\033[32mExpected error (symbol not found): "
                      << lib.getErrorMessage() << "\033[0m" << std::endl;
        }
    }
    catch (const dl::DynamicLibraryException& e)
    {
        std::cout << "\033[32mExpected error: " << e.what() << "\033[0m"
                  << std::endl;
    }
}

//-----------------------------------------------------------------------------
//! \brief Test if a library can be reloaded
//! \param p_library_path Path to the library
//! \return True if the library can be reloaded, false otherwise
//-----------------------------------------------------------------------------
static bool testLibraryUnloadability(const std::string& p_library_path)
{
    try
    {
        dl::DynamicLibrary testLib(p_library_path, dl::AutoReload::Enabled);
        return testLib.canReload();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
void example_reload_detection()
{
    std::cout << "\033[32m=== Example of reload detection ===\033[0m"
              << std::endl;

    // Test with different libraries
    std::vector<std::string> testLibs = {
        "./libexample" LIB_EXTENSION,
        "./libproblematic" LIB_EXTENSION, // Library with global constructors
                                          // globaux
        "./libstatic" LIB_EXTENSION       // Library with static variables
    };

    for (const auto& libPath : testLibs)
    {
        std::cout << "Testing: " << libPath << std::endl;

        if (testLibraryUnloadability(libPath))
        {
            std::cout << "\033[32m  ✓ Can be safely reloaded\033[0m"
                      << std::endl;
        }
        else
        {
            std::cout
                << "\033[31m  ⚠ Cannot be reloaded - potential issues:\033[0m"
                << std::endl;
            std::cout << "\033[31m    - Compiled with -Wl,-z,nodelete\033[0m"
                      << std::endl;
            std::cout << "\033[31m    - Global constructors without "
                         "destructors\033[0m"
                      << std::endl;
            std::cout << "\033[31m    - Static variables with complex "
                         "destructors\033[0m"
                      << std::endl;
            std::cout
                << "\033[31m    - Dependencies that cannot be unloaded\033[0m"
                << std::endl;
        }
    }
}

//-----------------------------------------------------------------------------
int main()
{
    example_basic_usage();
    example_hot_reload();
    example_manager();
    example_error_handling();
    example_reload_detection();

    return EXIT_SUCCESS;
}
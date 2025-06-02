//! ============================================================================
//! \file static_lib.cpp
//! \brief Library with complex static variables
//! ============================================================================

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

// Static variables with complex destructors
static std::vector<std::string> static_strings;
static std::mutex static_mutex;

// This class has a complex destructor
class ComplexResource
{
public:

    ComplexResource()
    {
        data.resize(1000);
        std::cout << "ComplexResource created" << std::endl;
    }

    ~ComplexResource()
    {
        std::cout << "ComplexResource destroyed" << std::endl;
        // Simulation of a complex cleanup that can fail
        data.clear();
    }

private:

    std::vector<int> data;
};

static ComplexResource complex_resource;

extern "C"
{
    void add_string(const char* str)
    {
        std::lock_guard<std::mutex> lock(static_mutex);
        static_strings.emplace_back(str);
    }

    size_t get_string_count()
    {
        std::lock_guard<std::mutex> lock(static_mutex);
        return static_strings.size();
    }

    const char* get_string(size_t index)
    {
        std::lock_guard<std::mutex> lock(static_mutex);
        if (index < static_strings.size())
        {
            return static_strings[index].c_str();
        }
        return nullptr;
    }
}
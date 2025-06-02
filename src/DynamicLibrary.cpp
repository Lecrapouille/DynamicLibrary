#include "DynamicLibrary/DynamicLibrary.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#    include <fileapi.h>
#    include <windows.h>
#    define NOMINMAX
using LibHandle = HMODULE;
#    define LIB_EXTENSION ".dll"
#else
#    include <dlfcn.h>
#    include <sys/stat.h>
#    include <unistd.h>
using LibHandle = void*;
#    ifdef __APPLE__
#        define LIB_EXTENSION ".dylib"
#    else
#        define LIB_EXTENSION ".so"
#    endif
#endif

namespace dl
{

//! ***************************************************************************
//! \brief Implementation of DynamicLibrary
//! ***************************************************************************
class DynamicLibrary::Implementation
{
public:

    //!------------------------------------------------------------------------
    //! \brief Library information
    //!------------------------------------------------------------------------
    struct LibraryInfo
    {
        LibHandle handle = nullptr;
        std::string path;
        std::chrono::system_clock::time_point last_modified;
        std::unordered_map<std::string, void*> symbol_cache;
        mutable bool reload_capability_tested = false;
        mutable bool can_reload = true;

        LibraryInfo() = default;
        LibraryInfo(const LibraryInfo&) = delete;
        LibraryInfo& operator=(const LibraryInfo&) = delete;

        //!--------------------------------------------------------------------
        //! \brief Move constructor
        //!--------------------------------------------------------------------
        LibraryInfo(LibraryInfo&& p_other) noexcept
            : handle(p_other.handle),
              path(std::move(p_other.path)),
              last_modified(p_other.last_modified),
              symbol_cache(std::move(p_other.symbol_cache)),
              reload_capability_tested(p_other.reload_capability_tested),
              can_reload(p_other.can_reload)
        {
            p_other.handle = nullptr;
        }

        //!--------------------------------------------------------------------
        //! \brief Move assignment operator
        //!--------------------------------------------------------------------
        LibraryInfo& operator=(LibraryInfo&& p_other) noexcept
        {
            if (this != &p_other)
            {
                handle = p_other.handle;
                path = std::move(p_other.path);
                last_modified = p_other.last_modified;
                symbol_cache = std::move(p_other.symbol_cache);
                reload_capability_tested = p_other.reload_capability_tested;
                can_reload = p_other.can_reload;
                p_other.handle = nullptr;
            }
            return *this;
        }
    };

    LibraryInfo lib;
    mutable std::mutex mutex;
    AutoReload auto_reload = AutoReload::Enabled;
    std::string error_message;

    //!------------------------------------------------------------------------
    //! \brief Validate the path of the library
    //! \param p_path Path of the library
    //!------------------------------------------------------------------------
    bool validatePath(const std::string& p_path)
    {
        if (p_path.empty())
        {
            error_message = "Library path cannot be empty";
            return false;
        }

        std::ifstream file(p_path);
        if (!file.good())
        {
            error_message =
                "Library file does not exist or is not accessible: " + p_path;
            return false;
        }
        return true;
    }

    //!------------------------------------------------------------------------
    //! \brief Get the file modification time
    //! \param p_path Path of the file
    //! \return The file modification time
    //!------------------------------------------------------------------------
    std::chrono::system_clock::time_point
    getFileModificationTime(const std::string& p_path) const
    {
#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA file_info;
        if (GetFileAttributesExA(
                p_path.c_str(), GetFileExInfoStandard, &file_info))
        {
            FILETIME ft = file_info.ftLastWriteTime;
            ULARGE_INTEGER ull;
            ull.LowPart = ft.dwLowDateTime;
            ull.HighPart = ft.dwHighDateTime;

            auto duration = std::chrono::nanoseconds(
                (ull.QuadPart - 116444736000000000ULL) * 100);
            return std::chrono::system_clock::time_point(duration);
        }
#else
        struct stat file_stat;
        if (stat(p_path.c_str(), &file_stat) == 0)
        {
            return std::chrono::system_clock::from_time_t(file_stat.st_mtime);
        }
#endif
        return std::chrono::system_clock::now();
    }

    //!------------------------------------------------------------------------
    //! \brief Load the library
    //!------------------------------------------------------------------------
    bool loadInternal()
    {
#ifdef _WIN32
        lib.handle = LoadLibraryA(lib.path.c_str());
        if (!lib.handle)
        {
            DWORD error = GetLastError();
            error_message = "Failed to load library '" + lib.path +
                            "' (Error: " + std::to_string(error) + ")";
            return false;
        }
#else
        lib.handle = dlopen(lib.path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!lib.handle)
        {
            std::string error = dlerror() ? dlerror() : "Unknown error";
            error_message =
                "Failed to load library '" + lib.path + "': " + error;
            return false;
        }
#endif
        return true;
    }

    //!------------------------------------------------------------------------
    //! \brief Unload the library
    //! \return True if successful, false otherwise
    //!------------------------------------------------------------------------
    bool unloadInternal()
    {
        if (!lib.handle)
            return true;

        lib.symbol_cache.clear();

#ifdef _WIN32
        bool success = FreeLibrary(lib.handle);
        if (!success)
        {
            DWORD error = GetLastError();
            error_message = "Failed to unload library '" + lib.path +
                            "' (Error: " + std::to_string(error) + ")";
        }
        lib.handle = nullptr;
        return success;
#else
        bool success = (dlclose(lib.handle) == 0);
        if (!success)
        {
            std::string error = dlerror() ? dlerror() : "Unknown error";
            error_message =
                "Failed to unload library '" + lib.path + "': " + error;
        }
        lib.handle = nullptr;
        return success;
#endif
    }

    //!------------------------------------------------------------------------
    //! \brief Get a symbol from the library
    //! \param p_symbol_name Name of the symbol to get
    //! \return The symbol
    //!------------------------------------------------------------------------
    void* getSymbolInternal(const std::string& p_symbol_name)
    {
#ifdef _WIN32
        void* symbol = reinterpret_cast<void*>(
            GetProcAddress(lib.handle, p_symbol_name.c_str()));
        if (!symbol)
        {
            error_message = "Symbol '" + p_symbol_name +
                            "' not found in library '" + lib.path + "'";
        }
        return symbol;
#else
        dlerror(); // Clear any previous error
        void* symbol = dlsym(lib.handle, p_symbol_name.c_str());
        char* error = dlerror();
        if (error)
        {
            error_message = "Symbol '" + p_symbol_name +
                            "' not found in library '" + lib.path +
                            "': " + error;
            return nullptr;
        }
        return symbol;
#endif
    }

    //!------------------------------------------------------------------------
    //! \brief Check if the library needs to be reloaded
    //! \return True if the library needs to be reloaded, false otherwise
    //!------------------------------------------------------------------------
    bool needsReload() const
    {
        auto current_mod_time = getFileModificationTime(lib.path);
        return current_mod_time > lib.last_modified;
    }

    //!------------------------------------------------------------------------
    //! \brief Test if the library can be reloaded (lazy evaluation)
    //! \return True if the library can be reloaded, false otherwise
    //!------------------------------------------------------------------------
    bool canReload() const
    {
        if (!lib.handle || lib.reload_capability_tested)
        {
            return lib.can_reload;
        }

        // Test once in a non-destructive way
        lib.reload_capability_tested = true;

#ifdef _WIN32
        // On can test by incrementing/decrementing the reference counter
        HMODULE test_handle = LoadLibraryA(lib.path.c_str());
        if (test_handle)
        {
            bool can_free = FreeLibrary(test_handle);
            lib.can_reload = can_free;
        }
        else
        {
            lib.can_reload = false;
        }
#else
        // On test with RTLD_NOLOAD to see if the lib is already loaded
        // then we try a quick dlopen/dlclose
        void* test_handle = dlopen(lib.path.c_str(), RTLD_NOW | RTLD_NOLOAD);
        if (test_handle)
        {
            // La lib est déjà en mémoire, on peut tester dlclose
            lib.can_reload = (dlclose(test_handle) == 0);
        }
        else
        {
            // Not in memory yet, we do a quick test
            test_handle = dlopen(lib.path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (test_handle)
            {
                lib.can_reload = (dlclose(test_handle) == 0);
            }
            else
            {
                lib.can_reload = false;
            }
        }
#endif

        return lib.can_reload;
    }

    //!------------------------------------------------------------------------
    //! \brief Reload the library
    //! \return True if successful, false otherwise
    //!------------------------------------------------------------------------
    bool reloadInternal()
    {
        // First check if the reload is possible
        if (!canReload())
        {
            error_message =
                "Library cannot be reloaded - reload capability not supported";
            return false;
        }

        std::string path = lib.path;

        // Attempt to unload
        if (!unloadInternal())
        {
            error_message = "Warning: Unload failed, attempting reload anyway";
        }

        // Small pause to let the system stabilize
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Reload
        lib.path = path;
        lib.last_modified = getFileModificationTime(path);

        bool success = loadInternal();
        if (!success)
        {
            error_message =
                "Failed to reload library '" + path + "': " + error_message;
        }

        return success;
    }

}; // DynamicLibrary::Implementation

//! ***************************************************************************
//! \brief Implementation of DynamicLibraryManager
//! ***************************************************************************
class DynamicLibraryManager::Implementation
{
public:

    std::unordered_map<std::string, std::unique_ptr<DynamicLibrary>>
        m_libraries;
    mutable std::mutex m_mutex;
};

//!----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary() noexcept
    : m_impl(std::make_unique<Implementation>())
{
}

//!----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary(const std::string& p_library_path,
                               AutoReload p_auto_reload)
    : m_impl(std::make_unique<Implementation>())
{
    if (!load(p_library_path, p_auto_reload))
    {
        throw DynamicLibraryException(m_impl->error_message);
    }
}

//!----------------------------------------------------------------------------
DynamicLibrary::~DynamicLibrary() = default;

//!----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary(DynamicLibrary&& p_other) noexcept
    : m_impl(std::move(p_other.m_impl))
{
}

//!----------------------------------------------------------------------------
DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& p_other) noexcept
{
    if (this != &p_other)
    {
        m_impl = std::move(p_other.m_impl);
    }
    return *this;
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::load(const std::string& p_library_path,
                          AutoReload p_auto_reload)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    if (m_impl->lib.handle)
    {
        m_impl->unloadInternal(); // On ignore le résultat
    }

    if (!m_impl->validatePath(p_library_path))
    {
        return false;
    }

    m_impl->lib.path = p_library_path;
    m_impl->lib.last_modified = m_impl->getFileModificationTime(p_library_path);
    m_impl->auto_reload = p_auto_reload;

    return m_impl->loadInternal();
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::unload()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->unloadInternal();
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::isLoaded() const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->lib.handle != nullptr;
}

//!----------------------------------------------------------------------------
void* DynamicLibrary::getSymbolInternal(const std::string& p_symbol_name)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    if (!m_impl->lib.handle)
    {
        m_impl->error_message = "Library not loaded";
        return nullptr;
    }

    if ((m_impl->auto_reload == AutoReload::Enabled) && m_impl->needsReload())
    {
        if (!m_impl->reloadInternal())
        {
            return nullptr;
        }
    }

    auto it = m_impl->lib.symbol_cache.find(p_symbol_name);
    if (it != m_impl->lib.symbol_cache.end())
    {
        return it->second;
    }

    void* symbol = m_impl->getSymbolInternal(p_symbol_name);
    if (symbol)
    {
        m_impl->lib.symbol_cache[p_symbol_name] = symbol;
    }

    return symbol;
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::checkForUpdates() const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->needsReload();
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::reload()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->lib.handle && m_impl->reloadInternal();
}

//!----------------------------------------------------------------------------
void DynamicLibrary::setAutoReload(AutoReload p_enable)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->auto_reload = p_enable;
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::canReload() const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->canReload();
}

//!----------------------------------------------------------------------------
std::string DynamicLibrary::getErrorMessage() const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->error_message;
}

//!----------------------------------------------------------------------------
bool DynamicLibrary::touch()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->lib.last_modified = std::chrono::system_clock::now();
    if (m_impl->auto_reload == AutoReload::Enabled)
    {
        return m_impl->reloadInternal();
    }
    return true;
}

//!----------------------------------------------------------------------------
DynamicLibraryManager::DynamicLibraryManager() noexcept
    : m_impl(std::make_unique<Implementation>())
{
}

//!----------------------------------------------------------------------------
DynamicLibraryManager::~DynamicLibraryManager() = default;

//!----------------------------------------------------------------------------
std::shared_ptr<DynamicLibrary>
DynamicLibraryManager::loadLibrary(const std::string& p_name,
                                   const std::string& p_path,
                                   AutoReload p_auto_reload)
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);

    auto it = m_impl->m_libraries.find(p_name);
    if (it != m_impl->m_libraries.end())
    {
        return std::shared_ptr<DynamicLibrary>(it->second.get(),
                                               [](DynamicLibrary*) {});
    }

    auto lib = std::make_unique<DynamicLibrary>(p_path, p_auto_reload);
    auto ptr = lib.get();
    m_impl->m_libraries[p_name] = std::move(lib);

    return std::shared_ptr<DynamicLibrary>(ptr, [](DynamicLibrary*) {});
}

//!----------------------------------------------------------------------------
void DynamicLibraryManager::unloadLibrary(const std::string& p_name)
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    m_impl->m_libraries.erase(p_name);
}

//!----------------------------------------------------------------------------
std::shared_ptr<DynamicLibrary>
DynamicLibraryManager::getLibrary(const std::string& p_name)
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    auto it = m_impl->m_libraries.find(p_name);
    if (it != m_impl->m_libraries.end())
    {
        return std::shared_ptr<DynamicLibrary>(it->second.get(),
                                               [](DynamicLibrary*) {});
    }
    return nullptr;
}

//!----------------------------------------------------------------------------
bool DynamicLibraryManager::checkAllForUpdates()
{
    std::lock_guard<std::mutex> lock(m_impl->m_mutex);
    for (const auto& library_pair : m_impl->m_libraries)
    {
        if (library_pair.second->checkForUpdates())
        {
            return true;
        }
    }
    return false;
}

} // namespace dl
#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

namespace dl
{

//! ***************************************************************************
//! \brief Enum class for auto-reload configuration
//! ***************************************************************************
enum class AutoReload
{
    Disabled, //!< Auto-reload is disabled
    Enabled   //!< Auto-reload is enabled
};

//! ***************************************************************************
//! \brief Exception class for DynamicLibrary errors
//! ***************************************************************************
class DynamicLibraryException: public std::runtime_error
{
public:

    explicit DynamicLibraryException(const std::string& p_message)
        : std::runtime_error(p_message)
    {
    }
};

//! ***************************************************************************
//! \brief Class for managing dynamic library loading and symbol resolution.
//! ***************************************************************************
class DynamicLibrary
{
public:

    //!------------------------------------------------------------------------
    //! \brief Default constructor. Nothing is loaded.
    //!------------------------------------------------------------------------
    DynamicLibrary() noexcept;

    //!------------------------------------------------------------------------
    //! \brief Constructor with automatic library loading.
    //! \param p_library_path Path to the library file.
    //! \param p_auto_reload Whether to enable automatic reloading.
    //! \throw DynamicLibraryException If the library fails to load.
    //!------------------------------------------------------------------------
    explicit DynamicLibrary(const std::string& p_library_path,
                            AutoReload p_auto_reload);

    //!------------------------------------------------------------------------
    //! \brief Destructor.
    //!------------------------------------------------------------------------
    ~DynamicLibrary();

    // Non-copyable but movable
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& p_other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& p_other) noexcept;

    //!------------------------------------------------------------------------
    //! \brief Load a dynamic library.
    //! \param p_library_path Path to the library file.
    //! \param p_auto_reload Whether to enable automatic reloading.
    //! \return true if the library was loaded successfully, false otherwise.
    //! \note The error message can be retrieved with getErrorMessage().
    //!------------------------------------------------------------------------
    bool load(const std::string& p_library_path, AutoReload p_auto_reload);

    //!------------------------------------------------------------------------
    //! \brief Unload the current library.
    //! \return true if the library was unloaded successfully, false otherwise.
    //! \note The error message can be retrieved with getErrorMessage().
    //!------------------------------------------------------------------------
    bool unload();

    //!------------------------------------------------------------------------
    //! \brief Check if a library is currently loaded.
    //! \return true if a library is loaded, false otherwise.
    //!------------------------------------------------------------------------
    bool isLoaded() const;

    //!------------------------------------------------------------------------
    //! \brief Get a symbol from the library.
    //! \tparam T Type of the symbol (function pointer type).
    //! \param p_symbol_name Name of the symbol to retrieve.
    //! \return The resolved symbol.
    //!------------------------------------------------------------------------
    template <typename T>
    T getSymbol(const std::string& p_symbol_name)
    {
        void* symbol = getSymbolInternal(p_symbol_name);
        return reinterpret_cast<T>(symbol);
    }

    //!------------------------------------------------------------------------
    //! \brief Get a function from the library.
    //! \tparam Func Function type.
    //! \param p_function_name Name of the function to retrieve.
    //! \return std::function wrapper around the function.
    //!------------------------------------------------------------------------
    template <typename Func>
    std::function<Func> getFunction(const std::string& p_function_name)
    {
        auto func_ptr = getSymbol<Func*>(p_function_name);
        return std::function<Func>(func_ptr);
    }

    //!------------------------------------------------------------------------
    //! \brief Check if the library has been updated.
    //! \return true if the library has been modified since last load.
    //!------------------------------------------------------------------------
    bool checkForUpdates() const;

    //!------------------------------------------------------------------------
    //! \brief Reload the library if it has been modified.
    //! \return true if the library was reloaded successfully, false otherwise.
    //! \note The error message can be retrieved with getErrorMessage().
    //!------------------------------------------------------------------------
    bool reload();

    //!------------------------------------------------------------------------
    //! \brief Enable or disable automatic reloading.
    //! \param p_enable Whether to enable automatic reloading.
    //!------------------------------------------------------------------------
    void setAutoReload(AutoReload p_enable = AutoReload::Enabled);

    //!------------------------------------------------------------------------
    //! \brief Check if the library can be reloaded.
    //! \return true if the library can be safely reloaded.
    //! \note The error message can be retrieved with getErrorMessage().
    //!------------------------------------------------------------------------
    bool canReload() const;

    //!------------------------------------------------------------------------
    //! \brief Get the path of the currently loaded library.
    //! \return The library path.
    //!------------------------------------------------------------------------
    std::string getPath() const;

    //!------------------------------------------------------------------------
    //! \brief Get the error message.
    //! \return The error message.
    //!------------------------------------------------------------------------
    std::string getErrorMessage() const;

    //!------------------------------------------------------------------------
    //! \brief Update the library's modification timestamp.
    //! \return true if the timestamp was updated successfully, false otherwise.
    //! \note This will trigger a reload if auto-reload is enabled.
    //!------------------------------------------------------------------------
    bool touch();

private:

    //!------------------------------------------------------------------------
    //! \brief Get a symbol from the library (internal implementation).
    //! \param p_symbol_name Name of the symbol to retrieve.
    //! \return Raw pointer to the symbol.
    //!------------------------------------------------------------------------
    void* getSymbolInternal(const std::string& p_symbol_name);

private:

    class Implementation;
    std::unique_ptr<Implementation> m_impl;
};

//! ***************************************************************************
//! \brief Manager class for multiple dynamic libraries.
//! ***************************************************************************
class DynamicLibraryManager
{
public:

    //!------------------------------------------------------------------------
    //! \brief Constructor.
    //!------------------------------------------------------------------------
    DynamicLibraryManager() noexcept;

    //!------------------------------------------------------------------------
    //! \brief Destructor.
    //!------------------------------------------------------------------------
    ~DynamicLibraryManager();

    //!------------------------------------------------------------------------
    //! \brief Load a library and store it in the manager.
    //! \param p_name Name to associate with the library.
    //! \param p_path Path to the library file.
    //! \param p_auto_reload Whether to enable automatic reloading.
    //! \return Shared pointer to the loaded library.
    //!------------------------------------------------------------------------
    std::shared_ptr<DynamicLibrary> loadLibrary(const std::string& p_name,
                                                const std::string& p_path,
                                                AutoReload p_auto_reload);

    //!------------------------------------------------------------------------
    //! \brief Unload a library from the manager.
    //! \param p_name Name of the library to unload.
    //!------------------------------------------------------------------------
    void unloadLibrary(const std::string& p_name);

    //!------------------------------------------------------------------------
    //! \brief Get a library from the manager.
    //! \param p_name Name of the library to retrieve.
    //! \return Shared pointer to the library, or nullptr if not found.
    //!------------------------------------------------------------------------
    std::shared_ptr<DynamicLibrary> getLibrary(const std::string& p_name);

    //!------------------------------------------------------------------------
    //! \brief Check all managed libraries for updates.
    //! \return True if any library has updates, false otherwise.
    //!------------------------------------------------------------------------
    bool checkAllForUpdates();

private:

    class Implementation;
    std::unique_ptr<Implementation> m_impl;
};

} // namespace dl
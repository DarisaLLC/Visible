
#ifndef _EXCEPTION_HPP
#define _EXCEPTION_HPP


#include <boost/current_function.hpp>
#include <stdexcept>
#include <string>

/*!
 * Define common exceptions used throughout the code:
 *
 * - The python built-in exceptions were used as inspiration.
 * - Exceptions inherit from std::exception to provide what().
 * - Exceptions inherit from civf::exception to provide code().
 *
 * The code() provides an error code which allows the application
 * the option of printing a cryptic error message from the 1990s.
 *
 * The dynamic_clone() and dynamic_throw() methods allow us to:
 * catch an exception by dynamic type (i.e. derived class), save it,
 * and later rethrow it, knowing only the static type (i.e. base class),
 * and then finally to catch it again using the derived type.
 *
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2106.html
 */

namespace svl {
    
    struct exception : std::runtime_error{
        exception(const std::string &what);
        virtual unsigned code(void) const = 0;
        virtual exception *dynamic_clone(void) const = 0;
        virtual void dynamic_throw(void) const = 0;
    };
    
    struct assertion_error : exception{
        assertion_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual assertion_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct lookup_error : exception{
        lookup_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual lookup_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct index_error : lookup_error{
        index_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual index_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct key_error : lookup_error{
        key_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual key_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct type_error : exception{
        type_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual type_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct value_error : exception{
        value_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual value_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct runtime_error : exception{
        runtime_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual runtime_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct not_implemented_error : runtime_error{
        not_implemented_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual not_implemented_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct environment_error : exception{
        environment_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual environment_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct io_error : environment_error{
        io_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual io_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct os_error : environment_error{
        os_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual os_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct system_error : exception{
        system_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual system_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct singular_error : exception{
        singular_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual singular_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    struct window_geometry_error : exception{
        window_geometry_error(const std::string &what);
        virtual unsigned code(void) const;
        virtual window_geometry_error *dynamic_clone(void) const;
        virtual void dynamic_throw(void) const;
    };
    
    
    /*!
     * Create a formated string with throw-site information.
     * Fills in the function name, file name, and line number.
     * \param what the std::exeption message
     * \return the formatted exception message
     */
#define ERROR_THROW_SITE_INFO(what) std::string( \
std::string(what) + "\n" + \
"  in " + std::string(BOOST_CURRENT_FUNCTION) + "\n" + \
"  at " + std::string(__FILE__) + ":" + BOOST_STRINGIZE(__LINE__) + "\n" \
)
} //namespace



#endif /* _EXCEPTION_HPP */

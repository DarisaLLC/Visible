
#ifndef _UTILS_PIMPL_HPP
#define _UTILS_PIMPL_HPP

#include <boost\shared_ptr.hpp>

/*! \file pimpl.hpp
 * "Pimpl idiom" (pointer to implementation idiom).
 * The PIMPL_* macros simplify code overhead for declaring and making pimpls.
 *
 * Each pimpl is implemented as a shared pointer to the implementation:
 * - The container class will not have to deallocate the pimpl.
 * - The container class will use the pimpl as a regular pointer.
 * - Usage: _impl->method(arg0, arg1)
 * - Usage: _impl->member = value;
 *
 * \see http://en.wikipedia.org/wiki/Opaque_pointer
 */

/*!
 * Make a declaration for a pimpl in a header file.
 * - Usage: PIMPL_DECL(impl) _impl;
 * \param _name the name of the pimpl class
 */
#define PIMPL_DECL(m_name) \
    struct m_name; boost::shared_ptr<m_name>

/*!
 * Make an instance of a pimpl in a source file.
 * - Usage: _impl = PIMPL_MAKE(impl, ());
 * - Usage: _impl = PIMPL_MAKE(impl, (a0, a1));
 * \param _name the name of the pimpl class
 * \param _args the constructor args for the pimpl
 */
#define PIMPL_MAKE(m_name, m_args) \
    boost::shared_ptr<m_name>(new m_name m_args)

#endif /* INCLUDED__UTILS_PIMPL_HPP */

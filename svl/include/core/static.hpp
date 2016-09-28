
#ifndef _UTILS_STATIC_HPP
#define _UTILS_STATIC_HPP


/*!
 * Defines a function that implements the "construct on first use" idiom
 * \param _t the type definition for the instance
 * \param _x the name of the defined function
 * \return a reference to the lazy instance
 */
#define SINGLETON_FCN(_t, _x) static _t &_x(){static _t _x; return _x;}

/*!
 * Defines a static code block that will be called before main()
 * The static block will catch and print exceptions to std error.
 * \param _x the unique name of the fixture (unique per source)
 */
#define STATIC_BLOCK(_x) \
    void _x(void); \
    static _static_fixture _x##_fixture(&_x, #_x); \
    void _x(void)

//! Helper for static block, constructor calls function
struct _static_fixture{
    _static_fixture(void (*)(void), const char *);
};

#endif /* _UTILS_STATIC_HPP */

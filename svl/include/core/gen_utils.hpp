//
//  gen_utils.hpp
//  Visible
//
//  Created by Arman Garakani on 6/6/19.
//

#ifndef gen_utils_h
#define gen_utils_h

#include <string>
#include <list>
#include <unistd.h>              //Needed for read.

using namespace std;

namespace svl{
    

    
#ifndef isininc
    //Inclusive_in_range()      isininc( 0, 10, 100) == true
    //                          isininc( 0, 100, 10) == false
    //                          isininc( 0, 10, 10)  == true
    //                          isininc( 10, 10, 10) == true
#define isininc( A, x, B ) (((x) >= (A)) && ((x) <= (B)))
#endif
    
    //For computing percent error relative to some known value. This should not be used for determining
    // equality of floating point values. See RELATIVE_DIFF for this purpose.
    //
    // NOTE: The 'known' or 'true' value should be supplied as parameter [B] here. Be aware
    //       that the concept of percent difference doesn't make much sense when the true
    //       value is zero (or near machine precision of zero). There is a computational
    //       singularity there. It is better to use the relative-difference in such
    //       situations, if possible.
    //
#ifndef PERCENT_ERR
#define PERCENT_ERR( A, B ) (100.0 * ((B) - (A)) / (B))
#endif

    //------------------------------------------------------------------------------------------------------
    //----------------------------------- Error/Warning/Verbosity macros -----------------------------------
    //------------------------------------------------------------------------------------------------------
    
    //------- Executable name variants.
#ifndef BASIC_ERRFUNC_
#define BASIC_ERRFUNC_
#define ERR( x )  { std::cerr << "--(E) " << argv[0] << ": " << x << ". Terminating program." << std::endl; \
std::cerr.flush();  \
std::exit(-1); }
#endif
    
#ifndef BASIC_WARNFUNC_
#define BASIC_WARNFUNC_
#define WARN( x )  { std::cout << "--(W) " << argv[0] << ": " << x << "." << std::endl; \
std::cout.flush();  }
#endif
    
#ifndef BASIC_INFOFUNC_
#define BASIC_INFOFUNC_
#define INFO( x )  { std::cout << "--(I) " << argv[0] << ": " << x << "." << std::endl; \
std::cout.flush();  }
#endif
    
    
    //------- Function name variants.
#ifdef __GNUC__ //If using gcc..
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __func__
#endif
#endif //__GNUC__
#ifndef __PRETTY_FUNCTION__   //(this is a fallback!)
#define __PRETTY_FUNCTION__ '(function name not available)'
#endif
    
    
#ifndef FUNCBASIC_ERRFUNC_
#define FUNCBASIC_ERRFUNC_
#define FUNCERR( x )  {std::cerr << "--(E) In function: " << __PRETTY_FUNCTION__; \
std::cerr <<  ": " << x << ". Terminating program." << std::endl; \
std::cerr.flush();  \
std::exit(-1); }
#endif
    
#ifndef FUNCBASIC_WARNFUNC_
#define FUNCBASIC_WARNFUNC_
#define FUNCWARN( x )  {std::cout << "--(W) In function: " << __PRETTY_FUNCTION__; \
std::cout <<  ": " << x << "." << std::endl; \
std::cout.flush();  }
#endif
    
    
#ifndef FUNCBASIC_INFOFUNC_
#define FUNCBASIC_INFOFUNC_
#define FUNCINFO( x )  {std::cout << "--(I) In function: " << __PRETTY_FUNCTION__; \
std::cout <<  ": " << x << "." << std::endl; \
std::cout.flush();  }
#endif

    //Checks if a variable, bitwise AND'ed with a bitmask, equals the bitmask. Ensure the ordering is observed
    // because the operation is non-commutative.
    //
    // For example: let A = 0110011
    //        and BITMASK = 0010010
    //   then A & BITMASK = 0010010 == BITMASK.
    //
    // For example: let A = 0100011
    //        and BITMASK = 0010010
    //   then A & BITMASK = 0000010 != BITMASK.
    //
    // NOTE: This operation is mostly useful for checking for embedded 'flags' within a variable. These flags
    // are set by bitwise ORing them into the variable.
    //
#ifndef BITMASK_BITS_ARE_SET
#define BITMASK_BITS_ARE_SET( A, BITMASK ) \
( (A & BITMASK) == BITMASK )
#endif
//------------------------------------------------------------------------------------------------------
//-------------------------------------- Function Declarations -----------------------------------------
//------------------------------------------------------------------------------------------------------


//Execute a given command in a read-only pipe (using popen/pclose.) Return the output.
// Do not use if you do not care about / do not expect output. An empty string should be
// able to be interpretted as a failure.
inline
std::string
Execute_Command_In_Pipe(const std::string &cmd){
    std::string out;
    auto pipe = popen(cmd.c_str(), "r");
    if(pipe == nullptr) return out;
    
    ssize_t nbytes;
    const long int buffsz = 5000;
    char buff[buffsz];
    
#ifdef EAGAIN
    while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1)  || (errno == EAGAIN) ){
#else
        while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1) ){
#endif
            //Check if we have reached the end of the file (ie. "data has run out.")
            if( nbytes == 0 ) break;
            
            //Otherwise we fill up the buffer to the high-water mark and move on.
            buff[nbytes] = '\0';
            out += std::string(buff,nbytes); //This is done so that in-buffer '\0's don't confuse = operator.
        }
        pclose(pipe);
        return out;
    }
    
    //------------------------------------------------------------------------------------------------------
    //-------------------------------------- Homogeneous Sorting -------------------------------------------
    //------------------------------------------------------------------------------------------------------
    //These routines can be delegated to to call the correct sorting routine for std::lists and non-
    // std::lists. The issue is that std::sort is not specialized to call std::list::sort() when it really
    // should.
    
    //---------------------------------------- Plain Functions ---------------------------------------------
    //Function defined when C is a std::list<>.
    template <typename C,
    typename std::enable_if<std::is_same<C,typename std::list<typename C::value_type>
    >::value
    >::type* = nullptr>
    void containerSort(C &c){
        c.sort();
    }
    //Function defined when C is NOT a std::list<>.
    template <typename C,
    typename std::enable_if<!std::is_same<C,typename std::list<typename C::value_type>
    >::value
    >::type* = nullptr>
    void containerSort(C &c){
        std::sort(c.begin(), c.end());
    }
    
    //-------------------------------------- Comparator Functions ------------------------------------------
    //Function defined when C is a std::list<>.
    template <typename C,
    typename Compare,
    typename std::enable_if<std::is_same<C,typename std::list<typename C::value_type>
    >::value
    >::type* = nullptr>
    void containerSort(C &c, Compare f){
        c.sort(f);
    }
    //Function defined when C is NOT a std::list<>.
    template <typename C,
    typename Compare,
    typename std::enable_if<!std::is_same<C,typename std::list<typename C::value_type>
    >::value
    >::type* = nullptr>
    void containerSort(C &c, Compare f){
        std::sort(c.begin(),c.end(),f);
    }
    
    
}
    

#endif /* gen_utils_h */

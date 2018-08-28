#ifndef __UT_TIME_BUFFER__
#define __UT_TIME_BUFFER__

#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <iostream>


#include <cassert>
#include "core/pair.hpp"

using namespace svl;

using namespace std;

namespace time_buffer_ut
{
    static void run ()
    {
        
            //[circular_buffer_example_2
            // Create a circular buffer with a capacity for 3 integers.
            boost::circular_buffer<int> cb(3);
            
            // Insert threee elements into the buffer.
            cb.push_back(1);
            cb.push_back(2);
            cb.push_back(3);
        
            for (auto bc : cb) std::cout  << " : " << bc << std::endl;
            int a = cb[0];  // a == 1
            int b = cb[1];  // b == 2
            int c = cb[2];  // c == 3
            
            // The buffer is full now, so pushing subsequent
            // elements will overwrite the front-most elements.
            
            cb.push_back(4);  // Overwrite 1 with 4.
            cb.push_back(5);  // Overwrite 2 with 5.
        
            for (auto bc : cb) std::cout  << " : " << bc << std::endl;
        
            // The buffer now contains 3, 4 and 5.
            a = cb[0];  // a == 3
            b = cb[1];  // b == 4
            c = cb[2];  // c == 5

            for (int i = 6; i < 20; i++)
            {
                cb.pop_front();
                cb.push_back (i);
                for (auto bc : cb) std::cout  << " : " << bc;
                std::cout << std::endl;
            }
            // Elements can be popped from either the front or the back.


        }
        
 };

#endif

/**
 * @file cpp-perf.hpp
 * @author Fabian Köhler fabian2804@googlemail.com
 * @version 0.1
 *
 * @section LICENSE
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Fabian Köhler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CPP_PERF_HPP
#define CPP_PERF_HPP

#include <chrono>
#include <algorithm>
#include <functional>
#include <ostream>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <thread>

namespace perf
{
    //===============================================
    //            Declaration of types
    //===============================================

    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;
    using hours = std::chrono::hours;
    using minutes = std::chrono::minutes;
    using seconds = std::chrono::seconds;
    using milliseconds = std::chrono::milliseconds;
    using microseconds = std::chrono::microseconds;
    using nanoseconds = std::chrono::nanoseconds;
    using perf_func = std::function<bool(void)>;

    /**
     * \struct perf_case
     * \brief A measurement case for suites
     */
    struct perf_case {
        perf_func f;
        std::string name;
        bool success;
        duration time;
    };

    // Forward declarations of classes
    class timer;
    class suite;
    class inline_timer;




    //===============================================
    //             Helper functions
    //===============================================
    
    /**
     * \brief Format a duration to be human readable.
     *
     * This function tries the different durations that are available through the
     * standard library. If the duration is longer than one hour the returned
     * string will represent the time rounded to hours. For shorter periods the
     * function will continue with minutes in the same manner.
     *
     * \param[in] time The time duration to format
     * \return The duration formatted as a string
     */
    std::string format_duration(const duration& time)
    {
        std::stringstream strm;

        auto h = std::chrono::duration_cast<hours>(time);
        if(h.count() >= 1.) {
            strm << h.count() << " h";
            return strm.str();
        }

        auto m = std::chrono::duration_cast<minutes>(time);
        if(m.count() >= 1.) {
            strm << m.count() << " min";
            return strm.str();
        }

        auto s = std::chrono::duration_cast<seconds>(time);
        if(s.count() >= 10.) {
            strm << s.count() << " s";
            return strm.str();
        }

        auto ms = std::chrono::duration_cast<milliseconds>(time);
        if(ms.count() >= 1.) {
            strm << ms.count() << " ms";
            return strm.str();
        }

        auto mus = std::chrono::duration_cast<microseconds>(time);
        if(mus.count() >= 1.) {
            strm << mus.count() << " \u03BCs";
            return strm.str();
        }
    
        auto ns = std::chrono::duration_cast<nanoseconds>(time);
        strm << ns.count() << " ns";
        return strm.str();
    }

    /**
     * \brief Format code positions.
     *
     * The function parameter is optional as not every code position must be in a function.
     * The string will have the following format:
     * file:[function:]line
     *
     * \param[in] file Path to the source file. You can use the __FILE__ macro
     * \param[in] line Line of code in the file. You can use the __LINE__ macro
     * \param[in] function The function containing this position. You can use the __FUNCTION__ macro
     * \return Code position formatted as a string.
     */
    std::string format_code_position(const std::string& file, std::size_t line, const std::string& function = "")
    {
        std::stringstream strm;
        strm << file << ":";
        if(!function.empty()) strm << function << ":";
        strm << line;
        return strm.str();
    }

    /**
     * \brief Append charaters to lengthen a string.
     *
     * The filler character will be appended to the string str until it reaches the length len
     *
     * \param[in,out] str The string which should be lengthened.
     * \param[in] len New length to achieve.
     * \param[in] filler The character to be used to lengthen the string
     */
    void lengthen_string(std::string& str, std::size_t len, char filler = ' ')
    {
        while(str.size() < len) str += filler;
    }



    //===============================================
    //             Class definitions
    //===============================================
   
    /**
     * \class timer
     * \brief Class to measure times.
     *
     * This class is used to manually time between the call of the start and stop member functions.
     */
    class timer
    {
        private:
            clock m_clock;
            time_point m_start, m_stop;

        public:
            /**
             * \brief Construct a timer object.
             * \param[in] start Decide if the timer should be created and started or only created.
             */
            timer(bool start = true)
            {
                if(start) m_start = m_clock.now();
            }

            /**
             * \brief This function starts the timer.
             */
            void start()
            {
                m_start = m_clock.now();
            }

            /**
             * \brief This function stops the timer.
             */
            void stop()
            {
                m_stop = m_clock.now();
            }

            /**
             * \brief This function returns the time interval between the start and the stop of the timer as a duration.
             * \return The elapsed time as a duration.
             */
            duration duration() const
            {
                return m_stop-m_start;
            }
    };

    /**
     * \class inline_timer
     * \brief Class used for automatic measurement using macros.
     *
     * This class is pobably of no use for you. It is used by the PERF_START()
     * and PERF_STOP() macros when using automatic measurement.
     */
    class inline_timer
    {
        friend std::ostream& operator<<(std::ostream& o, const inline_timer& timer);

        private:
            clock m_clock;
            time_point m_start, m_stop;
            std::string m_file;
            std::size_t m_first_line, m_last_line;
            std::string m_function;

        public:
            /**
             * \brief Constructor used to create inline timers
             *
             * \param[in] file The file where the inline timer is used in.
             * \param[in] line The first line of measurement. This is basically where the timer is created.
             * \param[in] function Optional parameter to indicate the function containing the measurement
             */
            inline_timer(std::string  file, std::size_t line, std::string  function = "") :
                m_file(std::move(file)), m_first_line(line), m_last_line(line), m_function(std::move(function)) {}

            /**
             * \brief This function starts the timer.
             */
            void start()
            {
                m_start = m_clock.now();
            }

            /**
             * \brief This function stops the timer.
             */
            void stop()
            {
                m_stop = m_clock.now();
            }

            /**
             * \brief Set the line where the measurement ends
             *
             * \param[in] line The number of the last line
             */
            void set_last_line(std::size_t line)
            {
                if(line > m_first_line) m_last_line = line;
            }
            
            /**
             * \brief This function returns the time interval between the start and the stop of the timer as a duration.
             * \return The elapsed time as a duration.
             */
            duration duration() const
            {
                return m_stop-m_start;
            }
    };

    /**
     * \brief Class to collect multiple test cases.
     *
     * Collect multiple test cases which are basically functions, functors or
     * lambdas with the signature bool(void). They can be ran all in order by
     * just one command. Exceptions will be caught and the observables success
     * and runtime are logged.
     *
     * An ostream operator helps you printing results out in a readable way.
     */
    class suite {
        friend std::ostream& operator<<(std::ostream& o, const suite& suite);

        private:
            std::vector<perf_case> m_cases;
            duration m_total_time;
            std::string m_name;

        public:

            /**
             * \brief Constructor to create an empty suite.
             *
             * This constructor creates a performance suite with a name.
             *
             * \param[in] name The name of the suite. The default is PerfSuite
             */
            suite(std::string  name = "PerfSuite") :
                m_name(std::move(name)) {}

            /**
             * \brief Constructor to create a suite with cases.
             *
             * This constructor creates a suite and adds all cases given as an argument.
             *
             * \param[in] cases The initial cases of the suite
             * \param[in] name The name of the suite. The default is PerfSuite
             */
            suite(const std::vector<std::pair<std::string, perf_func> >& cases, std::string  name = "PerfSuite") :
                m_name(std::move(name))
            {
                for(const auto& c : cases) add_case(c.first, c.second);
            }

            /**
             * \brief Add a case to the suite
             *
             * \param[in] name The name of the case.
             * \param[in] f The function, functor or lambda representing the case.
             */
            void add_case(const std::string& name, perf_func f)
            {
                m_cases.push_back({ f, name, false, duration() });
            }

            /**
             * \brief Set the name of a suite.
             *
             * \param[in] name The new name of the suite
             */
            void set_name(const std::string& name)
            {
                m_name = name;
            }

            /**
             * \brief Get the name of a suite.
             *
             * \return The name of the suite
             */
            std::string get_name() const
            {
                return m_name;
            }

            /**
             * \brief Execute all cases of the suite
             */
            void run()
            {
                clock clk;
                time_point start, stop;
                m_total_time = nanoseconds(0);
                
                for(auto& c : m_cases) {
                    start = clk.now();
                    try {
                        c.success = c.f();
                    } catch(...) {
                        c.success = false;
                    }
                    stop = clk.now();
                    c.time = stop-start;
                    m_total_time += c.time;
                }
            }
    };




    //===============================================
    //              ostream operators
    //===============================================
  
    /**
     * \brief Write a timer to an ostream.
     *
     * This operator writes a time to an ostream and formats the time using
     * the format_duration() function
     *
     * \param[in,out] o The ostream to use
     * \param[in] t The timer to output
     * \return Returns the ostream o
     */
    std::ostream& operator<<(std::ostream& o, const timer& t)
    {
        o << format_duration(t.duration());
        return o;
    }

    std::ostream& operator<<(std::ostream& o, const inline_timer& timer)
    {
        o << format_code_position(timer.m_file, timer.m_first_line, timer.m_function);
        if(timer.m_last_line > timer.m_first_line) o << "-" << timer.m_last_line;
        o << ": " << format_duration(timer.duration());
        return o;
    }

    std::ostream& operator<<(std::ostream& o, const suite& suite)
    {
        auto num_cases = suite.m_cases.size();
        auto num_cases_str = std::to_string(num_cases);

        auto max_name_length   = std::max_element(std::begin(suite.m_cases), std::end(suite.m_cases), [](const perf_case& a, const perf_case& b) {
            return a.name.size() < b.name.size();
        })->name.size();

        auto num = 0ul;
        auto num_str = std::string(), name_str = std::string(), success_str = std::string(), time_str = std::string();
        for(auto c : suite.m_cases) {
            num++;
            num_str = std::to_string(num);
            lengthen_string(num_str, num_cases_str.length());
            name_str = c.name + ' ';
            if(30 > max_name_length+1) {
                lengthen_string(name_str, 30, '.');
            } else {
                lengthen_string(name_str, max_name_length+1, '.');
            }
            if(c.success) success_str = "  Passed  ";
            else success_str = "  Failed  ";
            time_str = format_duration(c.time);

            o << num_str << '/' << num_cases_str << ' ' << "Case #" << num_str << ": " << name_str << success_str << time_str << std::endl;
        }

        return o;
    }




    //===============================================
    //        Wrappers for global variables
    //===============================================
    
    /**
     * \brief Return the ostream that is used for inline_timers
     *
     * This function returns the a pointer to the ostream that is used when PERF_STOP
     * prints out a inline_timer. This function can be used to redirect the output.
     *
     * \return Pointer to the std::ostream
     */
    std::ostream* inline_out_ptr()
    {
        static std::ostream* inline_out_ptr = &std::cout;
        return inline_out_ptr;
    }

    /**
     * \brief Return the ostream that is used for automatic performance suites
     *
     * This function returns a pointer to the std::ostream that is used when
     * PERF_END prints out the reults of an automatic performance suite
     *
     * \return Pointer to the std::ostream
     */
    std::ostream* auto_out_ptr()
    {
        static std::ostream* auto_out_ptr = &std::cout;
        return auto_out_ptr;
    }

    /**
     * \brief Implements a global timer stack.
     *
     * This function returs a reference to a static stack of timers.
     *
     * \return A reference to the stack of timers.
     */
    std::stack<timer>& timer_stack()
    {
        static std::stack<timer> timer_stack;
        return timer_stack;
    }

    /**
     * \brief Implements a global inline_timer stack.
     *
     * This function returns a reference to a static stack of inline_timers.
     *
     * \return A reference to the stack of inline_timers
     */
    std::stack<inline_timer>& inline_timer_stack()
    {
        static std::stack<inline_timer> inline_timer_stack;
        return inline_timer_stack;
    }

    /**
     * \brief Creates a new timer for quick and easy measurement.
     *
     * This function creates a new basic timer which on the timer stack and starts it.
     */
    void start()
    {
        timer_stack().push(timer());
    }
    
    /**
     * \brief Stops the last timer on the stack and prints the measured time.
     *
     * This function stops the last timer on the stack and prints the result on
     * the inline out ostream.
     */
    void stop()
    {
        timer_stack().top().stop();
        *inline_out_ptr() << timer_stack().top() << std::endl;
        timer_stack().pop();
    }

}




//===============================================
//                    Macros
//===============================================
#ifdef PERF_DISABLE_INLINE

#define PERF_START()
#define PERF_STOP()

#else

#define PERF_START() \
     perf::inline_timer_stack().push(perf::inline_timer(__FILE__, __LINE__, __FUNCTION__)); \
     perf::inline_timer_stack().top().start();

#define PERF_STOP() \
     perf::inline_timer_stack().top().stop(); \
     perf::inline_timer_stack().top().set_last_line(__LINE__); \
     *perf::inline_out_ptr() << perf::inline_timer_stack().top() << std::endl; \
     perf::inline_timer_stack().pop();

#endif

#define PERF_BEGIN(name) \
    int main() \
    { \
        perf::suite perf_auto_suite; \
        perf_auto_suite.set_name(name);


#define PERF_END() \
        perf_auto_suite.run(); \
        *perf::auto_out_ptr() << perf_auto_suite << std::endl; \
    }

#define PERF_CASE(name, ...) \
    perf_auto_suite.add_case(name, []() { \
        __VA_ARGS__ \
        return true; \
    });

#endif

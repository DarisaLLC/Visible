#ifndef _SCOPED_THREAD_
#define _SCOPED_THREAD_

#include <thread>

class scoped_thread
{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_): t (std::move(t_))
    {
        if (!t.joinable())
        {
            throw std::logic_error("scoped_thread constructed from non-joinable");
        }
    }

    ~scoped_thread()
    {
        if(t.joinable())
        {
            t.join();
        }
    }
    scoped_thread(scoped_thread const&)=delete;
    scoped_thread& operator=(scoped_thread const&)=delete;
};




#endif

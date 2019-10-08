//
//  task_manager.hpp
//  Visible
//
//  Created by Arman Garakani on 8/13/19.
//

#ifndef task_manager_hpp
#define task_manager_hpp

#include <functional>

namespace simple_task_pool
{
    struct manager
    {
        manager();
        ~manager();
    };
    
    void pump();
    
    void submit(std::function<void()> job, std::function<void()> on_complete);
}

#endif /* task_manager_hpp */

//
//  task_manager.cpp
//  Visible
//
//  Created by Arman Garakani on 8/13/19.
//


#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "task_manager.hpp"

#include <mutex>
#include <thread>

using namespace std;

namespace simple_task_pool
{
    namespace
    {
        int next_task_id = 0;
        
        struct task
        {
            std::function<void()> job, on_complete;
            std::thread thread;
        };
        
        const std::shared_ptr<std::mutex>& get_completed_tasks_mutex()
        {
            static std::shared_ptr<mutex> res = std::make_shared<mutex>();
            return res;
        }
        
        std::vector<int> completed_tasks;
        
        std::map<int, task> task_map;
        
        void run_task(std::function<void()> job, int task_id)
        {
            job();
            std::unique_lock<mutex> lck(*get_completed_tasks_mutex());
            completed_tasks.push_back(task_id);
        }
    }
    
    manager::manager()
    {
        get_completed_tasks_mutex();
    }
    
    manager::~manager()
    {
        while(task_map.empty() == false) {
            pump();
        }
    }
    
    void submit(std::function<void()> job, std::function<void()> on_complete)
    {
        std::function<void()> f = std::bind(run_task, job, next_task_id);
        task t = {
            job,
            on_complete,
            std::thread(f) };
        task_map[next_task_id].job = job;
        task_map[next_task_id].on_complete = on_complete;
        task_map[next_task_id].thread = thread(f);
        ++next_task_id;
    }
    
    void pump()
    {
        std::vector<int> completed;
        {
            std::unique_lock<std::mutex> lck(*get_completed_tasks_mutex());
            completed.swap(completed_tasks);
        }
        
        for(int t : completed) {
            task_map[t].on_complete();
            task_map.erase(t);
        }
    }
    
}

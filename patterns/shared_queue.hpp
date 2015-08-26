
#pragma once

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>

/** Multiple producer, multiple consumer thread safe queue
 * Since 'return by reference' is used this queue won't throw */
template<typename T>
class shared_queue {
   std::queue<T> queue_;
   mutable std::mutex m_;
   std::condition_variable data_cond_;

   shared_queue& operator=(const shared_queue&) = delete;
   shared_queue(const shared_queue& other) = delete;

public:

   shared_queue() {}

   void push(T item) {
      {
         std::lock_guard<std::mutex> lock(m_);
         queue_.push(std::move(item));
      }
      data_cond_.notify_one();
   }

   /// \return immediately, with true if successful retrieval
   bool try_and_pop(T& popped_item) {
      std::lock_guard<std::mutex> lock(m_);
      if (queue_.empty()) {
         return false;
      }
      popped_item = std::move(queue_.front());
      queue_.pop();
      return true;
   }

   /// Try to retrieve, if no items, wait till an item is available and try again
   void wait_and_pop(T& popped_item) {
      std::unique_lock<std::mutex> lock(m_);
      while (queue_.empty()) {
         data_cond_.wait(lock);
         //  This 'while' loop is equal to
         //  data_cond_.wait(lock, [](bool result){return !queue_.empty();});
      }
      popped_item = std::move(queue_.front());
      queue_.pop();
   }

   bool empty() const {
      std::lock_guard<std::mutex> lock(m_);
      return queue_.empty();
   }

   unsigned size() const {
      std::lock_guard<std::mutex> lock(m_);
      return queue_.size();
   }
};

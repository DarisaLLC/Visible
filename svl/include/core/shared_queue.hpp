
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

// concurrent Bounded Buffer. It’s a cyclic buffer with a certain capacity with a start and an end.
// http://baptiste-wicht.com/posts/2012/04/c11-concurrency-tutorial-advanced-locking-and-condition-variables.html


struct BoundedBuffer
{
    int* buffer;
    int capacity;
    
    int front;
    int rear;
    int count;
    
    std::mutex lock;
    
    std::condition_variable not_full;
    std::condition_variable not_empty;
    
    BoundedBuffer(int capacity) : capacity(capacity), front(0), rear(0), count(0) {
        buffer = new int[capacity];
    }
    
    ~BoundedBuffer(){
        delete[] buffer;
    }
    
    void deposit(int data){
        std::unique_lock<std::mutex> l(lock);
        
        not_full.wait(l, [this](){return count != capacity; });
        
        buffer[rear] = data;
        rear = (rear + 1) % capacity;
        ++count;
        
        not_empty.notify_one();
    }
    
    int fetch(){
        std::unique_lock<std::mutex> l(lock);
        
        not_empty.wait(l, [this](){return count != 0; });
        
        int result = buffer[front];
        front = (front + 1) % capacity;
        --count;
        
        not_full.notify_one();
        
        return result;
    }
};


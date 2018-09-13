
#pragma once

#include <queue>
#include <mutex>

namespace svl {

template<typename Data>
class ConcurrentQueue {
public:
	ConcurrentQueue( void ) {};
	~ConcurrentQueue( void ) {};

	void push( Data const& data )
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mQueue.push( data );
		lock.unlock();
		mCondition.notify_one();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lock( mMutex );
		return mQueue.empty();
	}

	bool try_pop( Data& popped_value )
	{
		std::lock_guard<std::mutex> lock( mMutex );
		if( mQueue.empty() ) {
			return false;
		}

		popped_value = mQueue.front();
		mQueue.pop();
		return true;
	}

	void wait_and_pop( Data& popped_value )
	{
		std::lock_guard<std::mutex> lock( mMutex );
		while( mQueue.empty() ) {
			mCondition.wait( lock );
		}

		popped_value = mQueue.front();
		mQueue.pop();
	}
private:
	std::queue<Data>			mQueue;
	mutable std::mutex		mMutex;
	std::condition_variable	mCondition;
};


template<typename Key, typename Data>
class ConcurrentMap {
public:
    ConcurrentMap( void ) {};
    ~ConcurrentMap( void ) {};
    
    void clear()
    {
        std::lock_guard<std::mutex> lock( mMutex );
        mQueue.clear();
    }
    
    bool contains( Key const& key )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::map<Key, Data>::iterator itr = mQueue.find( key );
        return ( itr != mQueue.end() );
    }
    
    bool erase( Key const& key )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        size_t n = mQueue.erase( key );
        
        return ( n > 0 );
    }
    
    void push( Key const& key, Data const& data )
    {
        std::unique_lock<std::mutex> lock( mMutex );
        mQueue[key] = data;
        lock.unlock();
        mCondition.notify_one();
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mQueue.empty();
    }
    
    bool get( Key const& key, Data& popped_value )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::map<Key, Data>::iterator itr = mQueue.find( key );
        if( itr == mQueue.end() )
            return false;
        
        popped_value = mQueue[key];
        
        return true;
    }
    
    bool try_pop( Key const& key, Data& popped_value )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::map<Key, Data>::iterator itr = mQueue.find( key );
        if( itr == mQueue.end() )
            return false;
        
        popped_value = mQueue[key];
        mQueue.erase( key );
        
        return true;
    }
    
    void wait_and_pop( Key const& key, Data& popped_value )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        typename std::map<Key, Data>::iterator itr;
        while( mQueue.find( key ) == mQueue.end() ) {
            mCondition.wait( lock );
        }
        
        popped_value = mQueue[key];
        mQueue.erase( key );
    }
private:
    std::map<Key, Data>            mQueue;
    mutable std::mutex            mMutex;
    std::condition_variable        mCondition;
};


template<typename Data>
class ConcurrentDeque {
public:
    ConcurrentDeque( void ) {};
    ~ConcurrentDeque( void ) {};
    
    void clear()
    {
        std::lock_guard<std::mutex> lock( mMutex );
        mDeque.clear();
    }
    
    bool contains( Data const& data )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
        return ( itr != mDeque.end() );
    }
    
    bool erase( Data const& data )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
        if( itr != mDeque.end() ) {
            mDeque.erase( itr );
            return true;
        }
        
        return false;
    }
    
    bool erase_all( Data const& data )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        
        typename std::deque<Data>::iterator itr;
        do {
            itr = std::find( mDeque.begin(), mDeque.end(), data );
            if( itr != mDeque.end() ) mDeque.erase( itr );
        } while( itr != mDeque.end() );
        
        return true;
    }
    
    bool push_back( Data const& data, bool unique = false )
    {
        std::unique_lock<std::mutex> lock( mMutex );
        
        if( unique ) {
            typename std::deque<Data>::iterator itr = std::find( mDeque.begin(), mDeque.end(), data );
            if( itr == mDeque.end() ) {
                mDeque.push_back( data );
                lock.unlock();
                mCondition.notify_one();
                
                return true;
            }
        }
        else {
            mDeque.push_back( data );
            lock.unlock();
            mCondition.notify_one();
            
            return true;
        }
        
        return false;
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mDeque.empty();
    }
    
    bool pop_front( Data& popped_value )
    {
        std::lock_guard<std::mutex> lock( mMutex );
        if( mDeque.empty() ) {
            return false;
        }
        
        popped_value = mDeque.front();
        mDeque.pop_front();
        return true;
    }
    
    void wait_and_pop_front( Data& popped_value )
    {
        std::unique_lock<std::mutex> lock( mMutex );
        while( mDeque.empty() ) {
            mCondition.wait( lock );
        }
        
        popped_value = mDeque.front();
        mDeque.pop_front();
    }
private:
    std::deque<Data>            mDeque;
    mutable std::mutex        mMutex;
    std::condition_variable    mCondition;
};

} // namespace svl

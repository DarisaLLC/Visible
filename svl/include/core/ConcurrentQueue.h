
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

} // namespace svl



#pragma once

#include <map>

namespace svl {

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
	std::map<Key, Data>			mQueue;
	mutable std::mutex			mMutex;
	std::condition_variable		mCondition;
};

} // namespace svl

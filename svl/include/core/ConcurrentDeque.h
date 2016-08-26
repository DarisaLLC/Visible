

#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

namespace svl {

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
	std::deque<Data>			mDeque;
	mutable std::mutex		mMutex;
	std::condition_variable	mCondition;
};

} // namespace svl
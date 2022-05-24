#ifndef __ASYNC_TRACKS__
#define __ASYNC_TRACKS__


#include <future>
#include <map>
#include "core/timestamp.h"
#include <optional>

typedef int cell_id_t;
typedef int contraction_id_t;


using namespace std;

typedef std::pair<int64_t, time_spec_t> index_time_t;


template<typename T>
class TimeTrack {
public:
	using vector_type    = std::vector<std::pair<double, T>>;
	using iterator       = typename vector_type::iterator;
	using const_iterator = typename vector_type::const_iterator;
	
	TimeTrack();
	
	void insert(double t, const T& o);
	
	iterator lower_bound(double t);
	iterator upper_bound(double t);
	
	const_iterator lower_bound(double t) const;
	const_iterator upper_bound(double t) const;
	
	const T& back() const;
	
	bool empty() const;
	
private:
	vector_type mTrack;
};


template<typename T>
struct KeyComp {
	bool operator()(const double& a, const std::pair<double, T>& b) {
		return a < b.first;
	}
	bool operator()(const std::pair<double, T>& a, const double& b) {
		return a.first < b;
	}
};

template<typename T>
TimeTrack<T>::TimeTrack()
{
	mTrack.reserve(1000000);
}

template<typename T>
void TimeTrack<T>::insert(double t, const T& o)
{
	mTrack.emplace(upper_bound(t), t, o);
}

template<typename T>
typename TimeTrack<T>::iterator TimeTrack<T>::lower_bound(double t)
{
	return std::lower_bound(mTrack.begin(), mTrack.end(), t, KeyComp<T>());
}

template<typename T>
typename TimeTrack<T>::iterator TimeTrack<T>::upper_bound(double t)
{
	return std::upper_bound(mTrack.begin(), mTrack.end(), t, KeyComp<T>());
}

template<typename T>
typename TimeTrack<T>::const_iterator TimeTrack<T>::lower_bound(double t) const
{
	return std::lower_bound(mTrack.begin(), mTrack.end(), t, KeyComp<T>());
}

template<typename T>
typename TimeTrack<T>::const_iterator TimeTrack<T>::upper_bound(double t) const
{
	return std::upper_bound(mTrack.begin(), mTrack.end(), t, KeyComp<T>());
}

template<typename T>
const T& TimeTrack<T>::back() const
{
	return mTrack.back().second;
}

template<typename T>
bool TimeTrack<T>::empty() const
{
	return mTrack.empty();
}



#endif


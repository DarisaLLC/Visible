#ifndef __PAIR_H
#define __PAIR_H

#include <cassert>

using namespace std;

template<class T>
class simple_pair : public  pair<T,T>

/* A simple_pair is a pair with pair-wise arithmetic operations
   added to the interface.

  T must have:
    T()
    T(const T&)
    ~T()
    T& operator= (const T&)

    operator+ (T, T)
    operator+= (T)
    operator- (T)
    operator- (T, T)
    operator-= (T)
    operator* (T, T)
    operator*= (T)
    operator/ (T, T)
    operator/= (T)
    operator== (T, T)

*/
{
public:
  simple_pair() :  pair<T,T>() {}
  simple_pair(T x, T y) :  pair<T,T>(x, y) {}
  simple_pair(T x) :  pair<T,T>(x, x) {}

  /* default copy ctor, assign, dtor OK */

  const T& x() const {return pair<T,T>::first;}
  T& x() {return pair<T,T>::first;}

  const T& y() const {return pair<T,T>::second;}
  T& y() {return pair<T,T>::second;}

  simple_pair<T> operator+ (const simple_pair<T>& r) const;
  simple_pair<T>& operator+= (const simple_pair<T>& r);

  simple_pair<T> operator- () const;
  simple_pair<T> operator- (const simple_pair<T>& r) const;
  simple_pair<T>& operator-= (const simple_pair<T>& r);

  simple_pair<T> operator* (const simple_pair<T>& r) const;
  simple_pair<T>& operator*= (const simple_pair<T>& r);

  simple_pair<T> operator/ (const simple_pair<T>& r) const;
  simple_pair<T>& operator/= (const simple_pair<T>& r);

  simple_pair<T> operator/ (const T& r) const;
  simple_pair<T>& operator/= (const T& r);

  T& operator [] (int d);
  T  operator [] (int d) const;

  bool operator== (const simple_pair<T>& r) const;
  bool operator!= (const simple_pair<T>& r) const { return !(*this == r); }

  friend ostream& operator<< (ostream& ous, const simple_pair<T>& dis)
  {
    ous << dis.x() << "," << dis.y();
    return ous;
  }

};

typedef simple_pair<uint32_t> UIPair;
typedef simple_pair<int32_t> IPair;
typedef simple_pair<float> FPair;
typedef simple_pair<double> DPair;
typedef simple_pair<bool> bPair;


template<class T>
simple_pair<T> simple_pair<T>::operator+ (const simple_pair<T>& r) const
{
  return simple_pair<T>(T(x() + r.x()), T(y() + r.y()));
}
template<class T>
simple_pair<T>& simple_pair<T>::operator+= (const simple_pair<T>& r)
{
  x() += r.x();
  y() += r.y();
  return *this;
}

template<class T>
T& simple_pair<T>::operator[] (int d)
{
  assert(d >= 0 && d < 2);
  return (!d) ? pair<T,T>::first : pair<T,T>::second;
}

template<class T>
T simple_pair<T>::operator[] (int d) const
{
  assert(d >= 0 && d < 2);
  return (!d) ? pair<T,T>::first : pair<T,T>::second;
}

template<class T>
simple_pair<T> simple_pair<T>::operator- () const
{
  return simple_pair<T>(T(-pair<T,T>::first), T(-pair<T,T>::second));
}

template<class T>
simple_pair<T> simple_pair<T>::operator- (const simple_pair<T>& r) const
{
  return simple_pair<T>(T(pair<T,T>::first - r.x()), T(pair<T,T>::second - r.y()));
}

template<class T>
simple_pair<T>& simple_pair<T>::operator-= (const simple_pair<T>& r)
{
  pair<T,T>::first -= r.x();
  pair<T,T>::second -= r.y();
  return *this;
}

template<class T>
simple_pair<T> simple_pair<T>::operator* (const simple_pair<T>& r) const
{
  return simple_pair<T>(T(pair<T,T>::first * r.x()), T(pair<T,T>::second * r.y()));
}

template<class T>
simple_pair<T>& simple_pair<T>::operator*= (const simple_pair<T>& r)
{
  pair<T,T>::first *= r.x();
  pair<T,T>::second *= r.y();
  return *this;
}

template<class T>
simple_pair<T> simple_pair<T>::operator/ (const simple_pair<T>& r) const
{
  return simple_pair<T>(T(pair<T,T>::first / r.x()), T(pair<T,T>::second / r.y()));
}

template<class T>
simple_pair<T>& simple_pair<T>::operator/= (const simple_pair<T>& r)
{
  pair<T,T>::first /= r.x();
  pair<T,T>::second /= r.y();
  return *this;
}

template<class T>
simple_pair<T> simple_pair<T>::operator/ (const T& r) const
{
  return simple_pair<T>(T(pair<T,T>::first / r), T(pair<T,T>::second / r));
}

template<class T>
simple_pair<T>& simple_pair<T>::operator/= (const T& r)
{
  pair<T,T>::first /= r;
  pair<T,T>::second /= r;
  return *this;
}
template<class T>
bool simple_pair<T>::operator== (const simple_pair<T>& r) const
{
  return pair<T,T>::first == r.x() && pair<T,T>::second == r.y();
}




#endif /* __PAIR_H */

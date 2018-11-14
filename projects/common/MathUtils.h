/*
 *  MathUtils.h
 *  cinder_dynmx
 *
 *  Created by Thomas Buhrmann on 24/01/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef _MATH_UTILS_
#define _MATH_UTILS_

//#include "Dynmx.h"
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <assert.h>
#include "float.h"

namespace dmx
{

const float PI = 3.1415926535897932384626433832795f;
const float TWO_PI = 6.283185307179586f;
const float PI_OVER_180 = 0.017453292519943295769236907684886f;
const float PI_OVER_180_REC = 57.295779513082325f;
const float PI_OVER_TWO = 1.570796326794897f;
const float PI_OVER_FOUR = 0.785398163397448f;

const float DEG_TO_RAD = PI / 180.0f;
const float RAD_TO_DEG = 180.0f / PI;
  
// TODO: MAX_FLOAT
#define MAX_NEG_FLOAT -FLT_MAX  

//----------------------------------------------------------------------------------------------------------------------
#ifdef DYNMX_WIN
static const float asinh(float v)
{
  return log(v + sqrt(v*v + 1));
}
#endif

//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline T clamp(T Value, T Min, T Max)
{
  return (Value < Min)? Min : (Value > Max)? Max : Value;
}
  
// Map one range to another
//----------------------------------------------------------------------------------------------------------------------
static double mapUnitIntervalToRange(double val, double min, double max)
{
  assert(max >= min);
  assert(val >=0.0 && val <= 1.0);
  return min + val * (max - min);
}

static double map01To(double val, double min, double max)
{
  assert(max >= min);
  assert(val >=0.0 && val <= 1.0);
  return min + val * (max - min);
}

static double mapTo01(double val, double min, double max)
{
  assert(max >= min);
  assert(val >=min && val <= max);
  return (val - min) / (max - min);
}
  
//----------------------------------------------------------------------------------------------------------------------
struct Range
{
  Range(double mi = 0.0, double ma = 1.0) : min(mi), max(ma) {};

  void set(double mi, double ma) { min = mi; max = ma; };
  
#if 0
  void toXml(ci::XmlTree& xml, const char* name) const 
  {  
    ci::XmlTree r (name, "");
    r.setAttribute("min", min);
    r.setAttribute("max", max); 
    xml.push_back(r);
  };
  
  void fromXml(const ci::XmlTree& parent, const char* name)
  {
    if(parent.hasChild(name))
    {
      min = parent.getChild(name).getAttributeValue<double>("min");
      max = parent.getChild(name).getAttributeValue<double>("max");
    }
  }
#endif
    
  double encode(double v) const { return mapTo01(v, min, max); };
  double decode(double v) const { return map01To(v, min, max); };
  double clamp(double v) const { return (v < min) ? min : (v > max) ? max : v;
 };
  
  double min, max;
};


static double map01To(double val, const Range& r)
{
  return map01To(val, r.min, r.max);
}
  
static double mapTo01(double val, const Range& r)
{
  return mapTo01(val, r.min, r.max);
}
  

//----------------------------------------------------------------------------------------------------------------------
static const float sqr(float v) { return v * v; };
static const double sqr(double v) { return v * v; };

//----------------------------------------------------------------------------------------------------------------------
static const float degreesToRadians(float deg) { return deg * DEG_TO_RAD; };
static const double degreesToRadians(double deg) { return deg * DEG_TO_RAD; };

//----------------------------------------------------------------------------------------------------------------------
static const float radiansToDegrees(float rad) { return rad * RAD_TO_DEG; };
static const double radiansToDegrees(double rad) { return rad * RAD_TO_DEG; };

//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline T wrap(T v, T min, T max)
{ 
  if(v > max)
  {
    return min + (v - max);
  }
  else if (v < min)
  {
    return max - (min - v);
  }

  return v;
}
  
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline int sign(T val)
{
  return val < 0 ? -1 : 1;
}
  
//----------------------------------------------------------------------------------------------------------------------  
static inline int even(int i)
{
  return i % 2 == 0;
}
  
static inline int odd(int i)
{
  return i % 2 != 0;
}
  
//----------------------------------------------------------------------------------------------------------------------
template <class T>
static inline std::string toString (T& t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}

// Create a 2d array with default values
// Receiver responsible for clean-up!
//----------------------------------------------------------------------------------------------------------------------
template<class T>
T** createArray2d(int n, int m, T init)
{
  T** a = new T* [n];
  for (int i = 0; i < n; ++i)
  {
    a[i] = new T [m];
    std::fill(a[i], a[i]+m, init);
  }
  
  return a;
}

// Fill a 2d array with same value
//----------------------------------------------------------------------------------------------------------------------
template<class T>
void fillArray2d(T** a, int n, int m, T val)
{
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m; ++j)
      a[i][j] = val;
}

// Print arrays
//----------------------------------------------------------------------------------------------------------------------
template<class T>
void printArray(T* a, int n, int prec=4)
{
  for (int i = 0; i < n; ++i)
  {
    std::cout << std::setprecision(prec) << std::fixed << a[i] << "\t";
  }
  std::cout << std::endl;
}
  
template<class T>
void printArray2d(T** a, int n, int m, int prec=4)
{
  for (int i = 0; i < n; ++i)
  {
    std::cout << i << ": \t";
    for (int j = 0; j < m; ++j)
    {
      std::cout << std::setprecision(prec) << std::fixed << a[i][j] << "\t";
    }
    std::cout << std::endl;
  }
}
  
#if 0
// Serialising of std::vec from and to ci::XmlTree
//----------------------------------------------------------------------------------------------------------------------
// Write vec to xml
template<class T>
void vecToXml(ci::XmlTree& xml, const std::vector<T>& vec, const std::string childName="Value")
{
  for(int i = 0; i < vec.size(); i++)
  {
    ci::XmlTree val (childName, toString(vec[i]));
    xml.push_back(val);
  }
}
  
// Add a std::vector to an xml tree
template<class T>
void vecToXml(ci::XmlTree& xml, const std::string vecName, const std::vector<T>& vec, const std::string childName="Value")
{
  ci::XmlTree newTree(vecName, "");
  vecToXml(newTree, vec, childName);
  xml.push_back(newTree);
}

// Read vec from given subtree of xml file
template<class T>
void vecFromXml(const ci::XmlTree& xml, std::vector<T>& vec, const std::string childName="Value")
{
  vec.clear();
  ci::XmlTree::ConstIter val = xml.begin(childName);
  for (; val != xml.end(); ++val)
  {
    const double v = val->getValue<T>();
    vec.push_back(v);
  }
}
  
// Read vec from xml given subtree name
template<class T>
void vecFromXml(const ci::XmlTree& xml, const std::string& name, std::vector<T>& vec, const std::string& childName="Value")
{
  if (xml.hasChild(name))
  {
    ci::XmlTree& subTree = SETTINGS->getChild(name);
    vecFromXml(subTree, vec, childName);
  }
}
#endif
    
// Vector sum
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline T sum(std::vector<T>& vec, int i0 = 0, int iN = 0)
{
  typename std::vector<T>::iterator first = vec.begin() + i0;
  typename std::vector<T>::iterator last = iN > 0 ? vec.begin() + iN : vec.end();
  
  T sum = std::accumulate(first, last, (T)0); 
  return sum;
}
  
// Vector max
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline T max(std::vector<T>& vec, int i0 = 0, int iN = 0)
{
  typename std::vector<T>::iterator first = vec.begin() + i0;
  typename std::vector<T>::iterator last = iN > 0 ? vec.begin() + iN : vec.end();
  
  return *std::max_element(first, last); 
}  

// Vector mean
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline T mean(std::vector<T>& vec, int i0 = 0, int iN = 0)
{
  int last = (iN == 0) ? vec.size() : iN;
  int N = last - i0;
  return sum(vec, i0, last) / (T) N;
}

// Standard deviation
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline void stdev(std::vector<T>& vec, T& m, T& sd, int i0 = 0, int iN = 0)
{
  int last = (iN == 0) ? vec.size() : iN;
  int N = last - i0;
  assert(N > 1);
  
  m = mean(vec, i0, last);
  
  T accum = 0;
  std::for_each(vec.begin() + i0, vec.begin() + last, [&](const double d)
  {
    accum += (d - m) * (d - m);
  });
  
  sd = sqrt(accum / (N-1));
}
  
// Element-wise sum of two vectors
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline void elemWiseSum(const std::vector<T>& vec1, const std::vector<T>& vec2, std::vector<T>& res)
{
  res.resize(vec1.size()); 
  std::transform(vec1.begin(), vec1.end(), vec2.begin(), res.begin(), std::plus<T>());
}

// Returns vector of successive differences, after transformation by operator.
//----------------------------------------------------------------------------------------------------------------------
template<class T, class Operator>
static inline void differences(const std::vector<T>& v1, std::vector<double>& res, Operator operate, 
                               bool padRight = true, int i0 = 0, int iN = 0)
{
  assert(v1.size() > 1);
  assert(i0 >= 0);
  
  if(iN == 0)
    iN = v1.size();
  
  assert(iN > i0 && iN <= v1.size());  

  // res will hold same amount of data as v1
  res.resize(iN-i0);
  res.clear();
  
  for (int i = i0; i < iN - 1; ++i) 
  {
    T diff = v1[i+1] - v1[i];
    res.push_back(operate(diff));
  }
  
  // Ensure same length by duplicating last element
  if(padRight)
    res.push_back(*res.end());
}

// Cross-correlation of two vectors
// (see http://paulbourke.net/miscellaneous/correlate/)
//----------------------------------------------------------------------------------------------------------------------
template<class T>
struct SqrDist
{
  double operator() (T t1, T t2) { return sqr(t1-t2); };
};


template<class T>
static inline std::vector<double> crossCorrelation(std::vector<T>& v1, std::vector<T>& v2, int minDelay, int maxDelay)
{  
  return crossCorrelation(v1, v2, SqrDist<T>(), minDelay, maxDelay);
}

// Cross-correlation of two vectors
// (see http://paulbourke.net/miscellaneous/correlate/)
//----------------------------------------------------------------------------------------------------------------------
template<class T, class Operator>
static inline std::vector<double> crossCorrelation(std::vector<T>& v1, std::vector<T>& v2, Operator operate, 
                                                   int minDelay, int maxDelay, 
                                                   int i0 = 0, int iN = 0)
{
  assert(maxDelay >= minDelay);
  assert(i0 >= 0);
  
  const int v1Size = v1.size();
  
  if(iN == 0)
    iN = v1Size;
  
  assert(iN > i0 && iN <= v1Size);
  
  // Calculate the correlation series
  std::vector<double> crossCor;
  for (int delay = minDelay; delay <= maxDelay; ++delay)
  {
    double corr = 0;
    for (int i = i0; i < iN; ++i) 
    {
      int j = i + delay;
      // For out of range values in second vector replace with first or last value
      T v2Val = j < i0 ? v2[i0] : j < iN ? v2[j] : v2[iN-1];
      double val = operate(v1[i], v2Val);
      corr += val;
    }
    crossCor.push_back(corr);
  }
  
  return crossCor;
}
  

// Cross-correlation of two vectors
// (see http://paulbourke.net/miscellaneous/correlate/)
//----------------------------------------------------------------------------------------------------------------------
template<class T>
static inline std::vector<T> crossCorrelationNormalised(std::vector<T>& v1, std::vector<T>& v2, int maxDelay)
{
  int N = v1.size();
  
  T m1 = mean(v1);
  T m2 = mean(v2);
  
  // Calculate the denominator
  T s1 = 0;
  T s2 = 0;
  for (int i = 0;i < N; i++) 
  {
    s1 += sqr(v1[i] - m1);
    s2 += sqr(v2[i] - m2);
  }
  T denom = sqrt(s1 * s2);  
  
  // Calculate the correlation series
  std::vector<T> crossCor;
  for (int delay = -maxDelay; delay < maxDelay; delay++) 
  {
    T s12 = 0;
    for (int i = 0; i < N; i++) 
    {
      int j = i + delay;
      if (j >= 0 && j < N)
      {
        s12 += (v1[i] - m1) * (v2[j] - m2);
      }
      else 
      {
        s12 += (v1[i] - m1) * (0 - m2);  
      }

    }
    crossCor.push_back(s12 / denom);
  }
}

// Smooth step according to Perlin (http://en.wikipedia.org/wiki/Smoothstep)
// Zero 1st and 2nd derivative at beginning and end
//----------------------------------------------------------------------------------------------------------------------
static const float smoothStep(float min, float max, float x)
{
  // Scale, and clamp x to 0..1 range
  x = clamp((x - min) / (max - min), 0.0f, 1.0f);
  // Evaluate polynomial
  return x*x*x*(x*(x*6 - 15) + 10);
}

static const float smoothStepUnitInterval(float x)
{
  return x*x*x*(x*(x*6 - 15) + 10);
}

static const double smoothStep(double min, double max, double x)
{
  // Scale, and clamp x to 0..1 range
  x = clamp((x - min) / (max - min), 0.0, 1.0);
  // Evaluate polynomial
  return x*x*x*(x*(x*6 - 15) + 10);
}

static const double smoothStepUnitInterval(double x)
{
  return x*x*x*(x*(x*6 - 15) + 10);
}

//----------------------------------------------------------------------------------------------------------------------
static const void secondsToTime(int time, int& hours, int& min, int& sec)
{
  hours = time / 3600;
  time = time % 3600;
  min = time / 60;
  time = time % 60;
  sec = time;  
}

//----------------------------------------------------------------------------------------------------------------------
// Fast random number generator from Numerical Recipes in C					            	
//----------------------------------------------------------------------------------------------------------------------
#define IAA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

// “Minimal” random number generator of Park and Miller with Bays-Durham shuffle and added safeguards. Returns a uniform 
// random deviate between 0.0 and 1.0 (exclusive of the endpoint values). Call with idum a negative integer to initialize; 
// thereafter, do not alter idum between successive deviates in a sequence. RNMX should approximate the largest floating 
// value that is less than 1.
//----------------------------------------------------------------------------------------------------------------------
static double ran1(long *idum)
{
	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	double temp;
	if (*idum <= 0 || !iy) 
  {
		if (-(*idum) < 1) 
      *idum = 1;
		else 
      *idum = -(*idum);
		for (j=NTAB+7; j>=0; j--) 
    {
			k = (*idum)/IQ;
			*idum=IAA*(*idum-k*IQ)-IR*k;
			if (*idum <0) 
        *idum += IM;
			if (j < NTAB) 
        iv[j] = *idum;
		}
		iy = iv[0];
	}
	k = (*idum)/IQ;
	*idum=IAA*(*idum-k*IQ)-IR*k;
	if (*idum < 0) 
    *idum += IM;
	j = iy/NDIV;
	iy=iv[j];
	iv[j] = *idum;
	if ((temp=AM*iy) > RNMX) 
    return RNMX;
	else 
    return temp;
}

//----------------------------------------------------------------------------------------------------------------------
static double gasdev(long *idum)
{
	static int iset=0;
	static double gset;
	double fac,rsq,v1,v2;
	if (iset == 0) 
  {
		do 
    {
			v1=2.0*ran1(idum)-1.0;
			v2=2.0*ran1(idum)-1.0;
			rsq=v1*v1+v2*v2;
		} while (rsq >= 1.0 || rsq==0.0);
		fac=sqrt(-2.0*log(rsq)/rsq);
		gset=v1*fac;
		iset=1;
		return v2*fac;
	} 
  else 
  {
		iset =0;
		return gset;
	}
}
  
} //namespace

#endif // _MATH_UTILS_

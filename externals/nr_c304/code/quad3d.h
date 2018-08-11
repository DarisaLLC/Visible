template <class T>
struct NRf3 {
	Doub xsav,ysav;
	T *func3d;
	Doub operator()(const Doub z)
	{
		return (*func3d)(xsav,ysav,z);
	}
};
template <class T, class Z1, class Z2>
struct NRf2 {
	NRf3<T> f3;
	Z1 &z1;
	Z2 &z2;
	NRf2(Z1 &zz1, Z2 &zz2) : z1(zz1), z2(zz2) {}
	Doub operator()(const Doub y)
	{
		f3.ysav=y;
		return qgaus(f3,z1(f3.xsav,y),z2(f3.xsav,y));
	}
};
template <class T, class Y1, class Y2, class Z1, class Z2>
struct NRf1 {
	Y1 &y1;
	Y2 &y2;
	NRf2<T,Z1,Z2> f2;
	NRf1(Y1 &yy1, Y2 &yy2, Z1 &z1, Z2 &z2) : y1(yy1),y2(yy2), f2(z1,z2) {}
	Doub operator()(const Doub x)
	{
		f2.f3.xsav=x;
		return qgaus(f2,y1(x),y2(x));
	}
};
template <class T, class Y1, class Y2, class Z1, class Z2>
Doub quad3d(T &func, const Doub x1, const Doub x2, Y1 &y1, Y2 &y2, Z1 &z1,
	Z2 &z2)
{
	NRf1<T,Y1,Y2,Z1,Z2> f1(y1,y2,z1,z2);
	f1.f2.f3.func3d=&func;
	return qgaus(f1,x1,x2);
}

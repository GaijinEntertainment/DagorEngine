#ifndef __UTILITY_EXTENSIONS_GENERATED_H__
#define __UTILITY_EXTENSIONS_GENERATED_H__

template< class T1, class T2, class T3 >
	struct triad {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;

		T1	first;
		T2	second;
		T3	third;

		triad() : first(T1()), second(T2()), third(T3()) {}
		triad(const T1& a, const T2& b, const T3& c) : first(a), second(b), third(c) {}
		template< class AT1, class AT2, class AT3 >
			triad(const triad< AT1, AT2, AT3 >& t) : first(t.first), second(t.second), third(t.third) {}
		~triad() {}
	};

template< class T1, class T2, class T3 >
	inline bool operator==(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third;
	}

template< class T1, class T2, class T3 >
	inline bool operator!=(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3 >
	inline bool operator<(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third ) );
	}

template< class T1, class T2, class T3 >
	inline bool operator>(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3 >
	inline bool operator<=(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3 >
	inline bool operator>=(const triad< T1, T2, T3 >& a, const triad< T1, T2, T3 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3 >
	triad< T1, T2, T3 > make_triad(const T1& a, const T2& b, const T3& c)
	{
		return triad< T1, T2, T3 >(a, b, c);
	}

template< class T1, class T2, class T3 >
	triad< T1, T2, T3 > m3(const T1& a, const T2& b, const T3& c)
	{
		return triad< T1, T2, T3 >(a, b, c);
	}

template< class T1, class T2, class T3, class T4 >
	struct quadruple {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;

		T1	first;
		T2	second;
		T3	third;
		T4	fourth;

		quadruple() : first(T1()), second(T2()), third(T3()), fourth(T4()) {}
		quadruple(const T1& a, const T2& b, const T3& c, const T4& d) : first(a), second(b), third(c), fourth(d) {}
		template< class AT1, class AT2, class AT3, class AT4 >
			quadruple(const quadruple< AT1, AT2, AT3, AT4 >& t) : first(t.first), second(t.second), third(t.third), fourth(t.fourth) {}
		~quadruple() {}
	};

template< class T1, class T2, class T3, class T4 >
	inline bool operator==(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third &&
		a.fourth == b.fourth;
	}

template< class T1, class T2, class T3, class T4 >
	inline bool operator!=(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3, class T4 >
	inline bool operator<(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third || (
		!(b.third < a.third) &&
			a.fourth < b.fourth ) ) );
	}

template< class T1, class T2, class T3, class T4 >
	inline bool operator>(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3, class T4 >
	inline bool operator<=(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3, class T4 >
	inline bool operator>=(const quadruple< T1, T2, T3, T4 >& a, const quadruple< T1, T2, T3, T4 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3, class T4 >
	quadruple< T1, T2, T3, T4 > make_quadruple(const T1& a, const T2& b, const T3& c, const T4& d)
	{
		return quadruple< T1, T2, T3, T4 >(a, b, c, d);
	}

template< class T1, class T2, class T3, class T4 >
	quadruple< T1, T2, T3, T4 > m4(const T1& a, const T2& b, const T3& c, const T4& d)
	{
		return quadruple< T1, T2, T3, T4 >(a, b, c, d);
	}

template< class T1, class T2, class T3, class T4, class T5 >
	struct quintuple {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;
		typedef T5 fifth_type;

		T1	first;
		T2	second;
		T3	third;
		T4	fourth;
		T5	fifth;

		quintuple() : first(T1()), second(T2()), third(T3()), fourth(T4()), fifth(T5()) {}
		quintuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e) : first(a), second(b), third(c), fourth(d), fifth(e) {}
		template< class AT1, class AT2, class AT3, class AT4, class AT5 >
			quintuple(const quintuple< AT1, AT2, AT3, AT4, AT5 >& t) : first(t.first), second(t.second), third(t.third), fourth(t.fourth), fifth(t.fifth) {}
		~quintuple() {}
	};

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator==(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third &&
		a.fourth == b.fourth &&
		a.fifth == b.fifth;
	}

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator!=(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator<(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third || (
		!(b.third < a.third) &&
			a.fourth < b.fourth || (
		!(b.fourth < a.fourth) &&
			a.fifth < b.fifth ) ) ) );
	}

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator>(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator<=(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3, class T4, class T5 >
	inline bool operator>=(const quintuple< T1, T2, T3, T4, T5 >& a, const quintuple< T1, T2, T3, T4, T5 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3, class T4, class T5 >
	quintuple< T1, T2, T3, T4, T5 > make_quintuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e)
	{
		return quintuple< T1, T2, T3, T4, T5 >(a, b, c, d, e);
	}

template< class T1, class T2, class T3, class T4, class T5 >
	quintuple< T1, T2, T3, T4, T5 > m5(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e)
	{
		return quintuple< T1, T2, T3, T4, T5 >(a, b, c, d, e);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	struct sextuple {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;
		typedef T5 fifth_type;
		typedef T6 sixth_type;

		T1	first;
		T2	second;
		T3	third;
		T4	fourth;
		T5	fifth;
		T6	sixth;

		sextuple() : first(T1()), second(T2()), third(T3()), fourth(T4()), fifth(T5()), sixth(T6()) {}
		sextuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f) : first(a), second(b), third(c), fourth(d), fifth(e), sixth(f) {}
		template< class AT1, class AT2, class AT3, class AT4, class AT5, class AT6 >
			sextuple(const sextuple< AT1, AT2, AT3, AT4, AT5, AT6 >& t) : first(t.first), second(t.second), third(t.third), fourth(t.fourth), fifth(t.fifth), sixth(t.sixth) {}
		~sextuple() {}
	};

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator==(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third &&
		a.fourth == b.fourth &&
		a.fifth == b.fifth &&
		a.sixth == b.sixth;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator!=(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator<(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third || (
		!(b.third < a.third) &&
			a.fourth < b.fourth || (
		!(b.fourth < a.fourth) &&
			a.fifth < b.fifth || (
		!(b.fifth < a.fifth) &&
			a.sixth < b.sixth ) ) ) ) );
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator>(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator<=(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	inline bool operator>=(const sextuple< T1, T2, T3, T4, T5, T6 >& a, const sextuple< T1, T2, T3, T4, T5, T6 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	sextuple< T1, T2, T3, T4, T5, T6 > make_sextuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f)
	{
		return sextuple< T1, T2, T3, T4, T5, T6 >(a, b, c, d, e, f);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6 >
	sextuple< T1, T2, T3, T4, T5, T6 > m6(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f)
	{
		return sextuple< T1, T2, T3, T4, T5, T6 >(a, b, c, d, e, f);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	struct septuple {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;
		typedef T5 fifth_type;
		typedef T6 sixth_type;
		typedef T7 seventh_type;

		T1	first;
		T2	second;
		T3	third;
		T4	fourth;
		T5	fifth;
		T6	sixth;
		T7	seventh;

		septuple() : first(T1()), second(T2()), third(T3()), fourth(T4()), fifth(T5()), sixth(T6()), seventh(T7()) {}
		septuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g) : first(a), second(b), third(c), fourth(d), fifth(e), sixth(f), seventh(g) {}
		template< class AT1, class AT2, class AT3, class AT4, class AT5, class AT6, class AT7 >
			septuple(const septuple< AT1, AT2, AT3, AT4, AT5, AT6, AT7 >& t) : first(t.first), second(t.second), third(t.third), fourth(t.fourth), fifth(t.fifth), sixth(t.sixth), seventh(t.seventh) {}
		~septuple() {}
	};

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator==(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third &&
		a.fourth == b.fourth &&
		a.fifth == b.fifth &&
		a.sixth == b.sixth &&
		a.seventh == b.seventh;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator!=(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator<(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third || (
		!(b.third < a.third) &&
			a.fourth < b.fourth || (
		!(b.fourth < a.fourth) &&
			a.fifth < b.fifth || (
		!(b.fifth < a.fifth) &&
			a.sixth < b.sixth || (
		!(b.sixth < a.sixth) &&
			a.seventh < b.seventh ) ) ) ) ) );
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator>(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator<=(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	inline bool operator>=(const septuple< T1, T2, T3, T4, T5, T6, T7 >& a, const septuple< T1, T2, T3, T4, T5, T6, T7 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	septuple< T1, T2, T3, T4, T5, T6, T7 > make_septuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g)
	{
		return septuple< T1, T2, T3, T4, T5, T6, T7 >(a, b, c, d, e, f, g);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
	septuple< T1, T2, T3, T4, T5, T6, T7 > m7(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g)
	{
		return septuple< T1, T2, T3, T4, T5, T6, T7 >(a, b, c, d, e, f, g);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	struct octuple {

		typedef T1 first_type;
		typedef T2 second_type;
		typedef T3 third_type;
		typedef T4 fourth_type;
		typedef T5 fifth_type;
		typedef T6 sixth_type;
		typedef T7 seventh_type;
		typedef T8 eighth_type;

		T1	first;
		T2	second;
		T3	third;
		T4	fourth;
		T5	fifth;
		T6	sixth;
		T7	seventh;
		T8	eighth;

		octuple() : first(T1()), second(T2()), third(T3()), fourth(T4()), fifth(T5()), sixth(T6()), seventh(T7()), eighth(T8()) {}
		octuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g, const T8& h) : first(a), second(b), third(c), fourth(d), fifth(e), sixth(f), seventh(g), eighth(h) {}
		template< class AT1, class AT2, class AT3, class AT4, class AT5, class AT6, class AT7, class AT8 >
			octuple(const octuple< AT1, AT2, AT3, AT4, AT5, AT6, AT7, AT8 >& t) : first(t.first), second(t.second), third(t.third), fourth(t.fourth), fifth(t.fifth), sixth(t.sixth), seventh(t.seventh), eighth(t.eighth) {}
		~octuple() {}
	};

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator==(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return
		a.first == b.first &&
		a.second == b.second &&
		a.third == b.third &&
		a.fourth == b.fourth &&
		a.fifth == b.fifth &&
		a.sixth == b.sixth &&
		a.seventh == b.seventh &&
		a.eighth == b.eighth;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator!=(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return !(a == b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator<(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return
		a.first < b.first || (
		!(b.first < a.first) &&
			a.second < b.second || (
		!(b.second < a.second) &&
			a.third < b.third || (
		!(b.third < a.third) &&
			a.fourth < b.fourth || (
		!(b.fourth < a.fourth) &&
			a.fifth < b.fifth || (
		!(b.fifth < a.fifth) &&
			a.sixth < b.sixth || (
		!(b.sixth < a.sixth) &&
			a.seventh < b.seventh || (
		!(b.seventh < a.seventh) &&
			a.eighth < b.eighth ) ) ) ) ) ) );
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator>(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return b < a;
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator<=(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return !(b > a);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	inline bool operator>=(const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& a, const octuple< T1, T2, T3, T4, T5, T6, T7, T8 >& b)
	{
		return !(a < b);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	octuple< T1, T2, T3, T4, T5, T6, T7, T8 > make_octuple(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g, const T8& h)
	{
		return octuple< T1, T2, T3, T4, T5, T6, T7, T8 >(a, b, c, d, e, f, g, h);
	}

template< class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8 >
	octuple< T1, T2, T3, T4, T5, T6, T7, T8 > m8(const T1& a, const T2& b, const T3& c, const T4& d, const T5& e, const T6& f, const T7& g, const T8& h)
	{
		return octuple< T1, T2, T3, T4, T5, T6, T7, T8 >(a, b, c, d, e, f, g, h);
	}


#endif

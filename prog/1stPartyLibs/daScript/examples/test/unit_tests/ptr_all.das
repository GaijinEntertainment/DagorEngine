
def test_signed( t : auto(TT) )
	// print("testing {typeinfo(typename type<TT>)} ")
	var zero = TT(0); zero--; zero++
	var one = TT(1); one--; one++
	var two = TT(2); two--; two++
	var three = TT(3); three--; three++
	var neg_one = TT(-1); neg_one--; neg_one++
	unsafe
		var a <- [[int 0;1;2;3]]
		var pA = addr(a[1])			// at 1
		assert(*pA == 1)
		pA ++						// at 2
		assert(*pA == 2)
		pA --						// at 1
		assert(*pA == 1)
		assert(pA[zero] == 1)		// at 1
		assert(pA[one] == 2)		// at 2
		assert(pA[neg_one] == 0)	// at 0
		pA += two					// at 3
		assert(*pA == 3)			// at 3
		pA -= three					// at 0
		assert(*pA == 0)			// at 0
		assert(*(pA + two) == 2)		// at 2
		pA += 2
		assert(*(pA - one) == 1)		// at 1
	// print("ok\n")

def test_unsigned( t : auto(TT) )
	// print("testing {typeinfo(typename type<TT>)} ")
	var zero = TT(0); zero--; zero++
	var one = TT(1); one--; one++
	var two = TT(2); two--; two++
	var three = TT(3); three--; three++
	unsafe
		var a <- [[int 0;1;2;3]]
		var pA = addr(a[1])			// at 1
		assert(*pA == 1)
		pA ++						// at 2
		assert(*pA == 2)
		pA --						// at 1
		assert(*pA == 1)
		assert(pA[zero] == 1)		// at 1
		assert(pA[one] == 2)		// at 2
		pA += two					// at 3
		assert(*pA == 3)			// at 3
		pA -= three					// at 0
		assert(*pA == 0)			// at 0
		assert(*(pA + two) == 2)		// at 2
		pA += 2
		assert(*(pA - one) == 1)		// at 1
	// print("ok\n")

[export]
def test
	test_signed(type<int>)
	test_signed(type<int64>)
	test_unsigned(type<uint>)
	test_unsigned(type<uint64>)
    return true




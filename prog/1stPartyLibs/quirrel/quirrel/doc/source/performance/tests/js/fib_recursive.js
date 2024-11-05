
// fibonacii test

function fibR(n) {
	if ( n<2 ) return n;
	return fibR(n-2) + fibR(n-1);
}

// infrastructure

function timeStamp() {
	return Date.now();
}


function profile(tname,cnt,testFn) {
	var t = 100500;
	var count = cnt
	while ( count>0 ) {
		var t0 = timeStamp();
		testFn();
		var t1 = timeStamp();
		var dt = t1 - t0;
		t = Math.min(dt, t);
		count --;
	}
	t /= 1000.0;
	var msg = '"'+tname+'", '+t+', '+cnt;
	print(msg)
}

function performance_tests() {
	profile("fibonacci recursive",20,function(){
		fibR(31)
	});
	timeStamp();
}

performance_tests();

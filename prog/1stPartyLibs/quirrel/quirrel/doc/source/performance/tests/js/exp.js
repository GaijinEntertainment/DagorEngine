// exponent test

function expLoop(n) {
	var sum = 0;
	for ( var i=0; i!=n; ++i ) {
		sum += Math.exp(1.0/(1.0+i));
	}
	return sum;
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
	profile("exp loop",20,function(){
		expLoop(1000000);
	});
	timeStamp();
}

performance_tests();

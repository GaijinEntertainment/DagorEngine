// fibonacii test

function fibI(n) {
	var last = 0
	var cur = 1
	n = n - 1
	while ( n>0 ) {
		n = n - 1
		var tmp = cur
		cur = last + cur
		last = tmp
	}
	return cur
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
	profile("fibonacci loop",20,function(){
		fibI(6511134);
	});
	timeStamp();
}

performance_tests();

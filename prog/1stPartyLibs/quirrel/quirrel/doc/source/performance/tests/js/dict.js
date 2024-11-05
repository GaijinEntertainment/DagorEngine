// dictionary test

function dict_makeSrc(){
	var src = [];
	var n = 500000;
	var modn = n;
	for (var i=0; i != n; ++i ) {
		var num = (271828183 ^ i*119) % modn;
		src.push('_' + num);
	}
	return src;
}

function dict(src) {
	var tab = {}
	var n = src.length;
	var max = 1
	for (var i=0; i != n; ++i ) {
		var l = src[n];
		if ( tab.hasOwnProperty(l) ) {
			max = Math.max(++tab[l],max);
		} else {
			tab[l] = 1;
		}
	}
	return max;
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
	{
		var src = dict_makeSrc();
		profile("dictionary",20,function(){
			dict(src);
		});
	}
	timeStamp();
}

performance_tests();

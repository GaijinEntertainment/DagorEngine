// particle test


let TOTAL_NUMBERS = 10000;

let TOTAL_TIMES = 100;

function mk_float(i) {
    return i + (i / TOTAL_NUMBERS);
}

function update(nums) {
    let summ = 0;
    for (let i = 1; i <= TOTAL_TIMES; i++) {
        for (let j = 0; j < nums.length; j++) {
            summ = summ + nums[j].toString().length;
        }
    }
    return summ;
}

function make_nums() {
    let nums = [];
    for (let i = 1; i <= TOTAL_NUMBERS; i++) {
        nums.push(mk_float(i));
    }
    return nums;
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
        let nums = make_nums();
		profile("float2string",1,function(){
			update(nums);
		});
	}
	timeStamp();
}

performance_tests();

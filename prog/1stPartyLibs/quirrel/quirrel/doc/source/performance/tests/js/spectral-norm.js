function A(i,j) {
return 1/((i+j)*(i+j+1)/2+i+1);
}

function Au(u,v) {
for (var i=0; i<u.length; ++i) {
    var t = 0;
    for (var j=0; j<u.length; ++j)
    t += A(i,j) * u[j];
    v[i] = t;
}
}

function Atu(u,v) {
for (var i=0; i<u.length; ++i) {
    var t = 0;
    for (var j=0; j<u.length; ++j)
    t += A(j,i) * u[j];
    v[i] = t;
}
}

function AtAu(u,v,w) {
Au(u,w);
Atu(w,v);
}

function spectralnorm(n) {
var i, u=[], v=[], w=[], vv=0, vBv=0;
for (i=0; i<n; ++i) {
    u[i] = 1; v[i] = w[i] = 0;
}
for (i=0; i<10; ++i) {
    AtAu(u,v,w);
    AtAu(v,u,w);
}
for (i=0; i<n; ++i) {
    vBv += u[i]*v[i];
    vv  += v[i]*v[i];
}
return Math.sqrt(vBv/vv);
}

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
		profile("spectral norm",10,function(){
			spectralnorm(500);
		});
	}
	timeStamp();
}

performance_tests();
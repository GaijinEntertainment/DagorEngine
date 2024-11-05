// particle test

function update_particle(p) {
	p.pos.x += p.vel.x;
	p.pos.y += p.vel.y;
	p.pos.z += p.vel.z;
}

function update_particles(particles) {
	var n = particles.length;
	for ( var i=0; i!=n; ++i ) {
		update_particle(particles[i]);
	}
}

function multi_update_particles(particles,count) {
	while ( count ) {
		update_particles(particles);
		count--;
	}
}

function update_particles_i(particles) {
	var n = particles.length;
	for ( var i=0; i!=n; ++i ) {
		var p = particles[i];
		p.pos.x += p.vel.x;
		p.pos.y += p.vel.y;
		p.pos.z += p.vel.z;
	}
}

function multi_update_particles_i(particles,count) {
	while ( count ) {
		update_particles_i(particles);
		count--;
	}
}

function make_particles() {
	var particles = []
	var n = 50000;
	for ( var i=0; i!=n; ++i ) {
		var p = {
			pos : {x : i + 0.1, y : i + 0.2, z : i + 0.3},
			vel : {x : 1.1, y : 2.1,  z : 3.1}
		};
		particles.push(p);
	}
	return particles;
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
        var particles = make_particles();
		profile("particles kinematics",20,function(){
			multi_update_particles_i(particles,100);
		});
	}
	timeStamp();
}

performance_tests();

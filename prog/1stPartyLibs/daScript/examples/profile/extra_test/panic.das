require testProfile

[sideeffects]
def tryCatch()
	let arr:int[1000]
	var cnt = 0
	for j in range (0, 100)
		var fail = 0
		for i in range (0, 2000)
			try
				cnt += arr[i]
			recover
				fail ++
		assert(fail==1000)


[export]
def main
	profile(20,"try-recover") <|
		tryCatch()
	profile(20,"try-recover c++") <|
		testProfile::testTryCatch()


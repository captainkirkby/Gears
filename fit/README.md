Functional Test:

	./fit.py --load-template NotchedFingers.template < ~/runningDataWithGlitch.dat > tmp

	diff tmp NotchedFinters.out

	rm tmp
	
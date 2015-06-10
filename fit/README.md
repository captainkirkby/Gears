Functional Test:

	./fit.py --load-template NotchedFingers.template < ~/runningDataWithGlitch.dat > tmp

	diff tmp NotchedFinters.out

	rm tmp
	
Unit Tests:

	python -m unittest test_Frame test_Template test_DB

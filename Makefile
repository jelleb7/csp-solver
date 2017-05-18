all: 
	cd sources/basicsolver/ && make
	cd sources/cspsolver/ && make
	cp sources/basicsolver/solver ./solver
	cp sources/cspsolver/csp ./csp	
clean:
	cd sources/basicsolver/ && make clean
	cd sources/cspsolver/ && make clean
	rm ./solver ./csp




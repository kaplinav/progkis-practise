default:
	g++ -o prover circuit.cpp

clean:
	rm -f prover *.out *.vars

all: 

	mpicc toroide.c -o Toroide
	mpicc hipercubo.c -o Hipercubo

run-toroide:

	mpirun -n 9 ./Toroide
	
run-hipercubo:

	mpirun -n 8 ./Hipercubo
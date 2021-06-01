warehousesim: warehousesim.c warehousesim.h
	gcc -pthread -o warehousesim warehousesim.c 

clean: 
	rm -f *.o
	rm -f warehousesim

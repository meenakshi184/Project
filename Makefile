debug :
	g++ -g -c wifi4.cpp
	g++ -Wall wifi4.o -o wifi4_debug

optmize:
	g++ -O2 -c wifi4.cpp
	g++ -Wall wifi4.o -o wifi4_opt	

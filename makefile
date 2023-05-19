all:
	g++ -std=c++11 ./src/main.cpp ./src/CustomMdSpi.cpp ./src/CTPAPI/thostmduserapi_se.so -I ./src -o test
clean:
	rm test flow/*.con stats/*.csv
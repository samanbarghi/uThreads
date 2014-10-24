INCLUDE_IDR=include
BUILD_DIR=build
BIN_DIR=bin
SRC_DIR=src
LIB_DIR=lib
SPIKE_DIR=spike

CXX=g++ -std=c++1y

lib: $(SRC_DIR)/uThreadLib.cpp $(SRC_DIR)/uThreadLib.h
	$(CXX) -c -fPIC $(SRC_DIR)/uThreadLib.cpp -o $(BUILD_DIR)/uThreadLib.o
	$(CXX) -shared -o $(LIB_DIR)/libuThread.so $(BUILD_DIR)/uThreadLib.o 
	
libTest: $(SPIKE_DIR)/libTest.cpp
	$(CXX) -o $(BIN_DIR)/libTest $(SPIKE_DIR)/libTest.cpp -Llib -luThread
	
clean:
	rm -f $(BUILD_DIR)/*
	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/*
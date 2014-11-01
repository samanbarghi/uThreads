BUILD_DIR=build
BIN_DIR=bin
SRC_DIR=src
LIB_DIR=lib
SPIKE_DIR=spike
INCLUDE_IDR=include

CXX		 := g++ -std=c++1y
CXXFLAGS := -g -ggdb -m64

SRCEXT 	:= cpp
ASMEXT 	:= S

SOURCES := $(shell find $(SRC_DIR) -type f -name *.$(SRCEXT))
SSOURCES:= $(shell find $(SRC_DIR) -type f -name *.$(ASMEXT))

OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SOURCES:.$(SRCEXT)=.o))
SOBJECTS:= $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SSOURCES:.$(ASMEXT)=.o))

LIB 	:= -pthread
INC		:= -I include
TARGET	:= $(LIB_DIR)/libuThread.so

all:	$(TARGET) spike
$(TARGET) :  $(SOBJECTS) $(OBJECTS)
	@echo " Linking..."
	$(CXX) -shared -m64 -fPIC $^ -o $(TARGET) $(LIB)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRCEXT)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) -c -fPIC -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(ASMEXT)
	$(CXX) $(INC) -c -m64 -shared -fPIC -o $@ $<

#spikes
spike: $(SPIKE_DIR)/libTest.cpp
	$(CXX)  $(CXXFLAGS) $(INC) -L$(LIB_DIR) -o $(BIN_DIR)/libTest $(SPIKE_DIR)/libTest.cpp -luThread $(LIB)

clean:
	@echo " Cleaning..."
	rm -rf $(BUILD_DIR)/*
	rm -rf $(BIN_DIR)/*
	rm -rf $(LIB_DIR)/*

BUILD_DIR=build
BIN_DIR=bin
SRC_DIR=src
LIB_DIR=lib
SPIKE_DIR=spike
INCLUDE_IDR=include

CXX		 := g++ -std=c++1y
#CXX		 := clang -std=c++1y
CXXFLAGS := -O3 -g -ggdb -m64 -fpermissive -mtls-direct-seg-refs -fno-extern-tls-init -pthread

SRCEXT 	:= cpp
ASMEXT 	:= S

SOURCES := $(shell find $(SRC_DIR) -type f -name *.$(SRCEXT))
SSOURCES:= $(shell find $(SRC_DIR) -type f -name *.$(ASMEXT))
SPIKESOURCES := $(shell find $(SPIKE_DIR) -type f -name *.$(SRCEXT))

OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SOURCES:.$(SRCEXT)=.o))
SOBJECTS:= $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SSOURCES:.$(ASMEXT)=.o))
SPIKEOBJECTS := $(patsubst $(SPIKE_DIR)/%,$(BIN_DIR)/%,$(SPIKESOURCES:.$(SRCEXT)=))

LIB 	:= -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -ldl
INC		:= -I include
TARGET	:= $(LIB_DIR)/libuThread.so

all:	$(TARGET) $(SPIKEOBJECTS)
$(TARGET) :  $(SOBJECTS) $(OBJECTS)
	@echo " Linking..."
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -m64 -fPIC $^ -o $(TARGET) $(LIB)
-include $(OBJECTS:.o=.d)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRCEXT)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/io
	$(CXX) $(CXXFLAGS) -MMD $(INC) -c -fPIC -o $@ $<
	@mv -f $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.d.tmp
	@sed -e 's|.*:|$(BUILD_DIR)/$*.o:|' < $(BUILD_DIR)/$*.d.tmp > $(BUILD_DIR)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILD_DIR)/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILD_DIR)/$*.d
	@rm -f $(BUILD_DIR)/$*.d.tmp

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(ASMEXT)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(INC) -c -m64 -shared -fPIC -o $@ $<

#spikes
$(BIN_DIR)/%: $(SPIKE_DIR)/%.$(SRCEXT)
	@echo "$@ $<"
	@mkdir -p $(BIN_DIR)
	$(CXX)  $(CXXFLAGS) $(INC) -L$(LIB_DIR) -o $@ $< -luThread

clean:
	@echo " Cleaning..."
	rm -rf $(BUILD_DIR)/*
	rm -rf $(BIN_DIR)/*
	rm -rf $(LIB_DIR)/*
# DO NOT DELETE

install:
	cp lib/libuThread.so /usr/lib

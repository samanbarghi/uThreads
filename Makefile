BUILD_DIR=src
BIN_DIR=bin
SRC_DIR=src
INCLUDE_DIR=include
LIB_DIR=lib
TEST_DIR=test
DEST_DIR=/usr/local

VERSION_MAJOR=0
VERSION_MINOR=2
VERSION_PATCH=0
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

LIB_NAME=libuThreads.so
LIB_FULL_NAME=$(LIB_NAME).$(VERSION)

CXX		 := g++ -std=c++1y
CXXFLAGS := -O3 -g -m64 -fpermissive -mtls-direct-seg-refs -fno-extern-tls-init -pthread -DNDEBUG

SRCEXT 	:= cpp
ASMEXT 	:= S

SOURCES := $(shell find $(SRC_DIR) -type f -name *.$(SRCEXT))
SSOURCES:= $(shell find $(SRC_DIR) -type f -name *.$(ASMEXT))
TESTSOURCES := $(shell find $(TEST_DIR) -type f -name *.$(SRCEXT))

OBJECTS := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SOURCES:.$(SRCEXT)=.o))
SOBJECTS:= $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(SSOURCES:.$(ASMEXT)=.o))
TESTOBJECTS := $(patsubst $(TEST_DIR)/%,$(BIN_DIR)/%,$(TESTSOURCES:.$(SRCEXT)=))

LIB 	:= -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -ldl -Wl,-soname,$(LIB_NAME).$(VERSION_MAJOR)
INC		:= -I $(SRC_DIR) -I $(INCLUDE_DIR)
TARGET	:= $(LIB_DIR)/$(LIB_FULL_NAME)

HTTP_PARSER := test/include/http_parser.c

all: $(TARGET)
	@mkdir -p $(BUILD_DIR)/io
	@mkdir -p $(BUILD_DIR)/runtime
	@mkdir -p $(BUILD_DIR)/generic

#Pass O1 to linker to optimize the hash table size, can be verified by `readelf -I`
$(TARGET) :  $(SOBJECTS) $(OBJECTS)
	@echo " Linking..."
	@mkdir -p $(LIB_DIR)
	$(CXX) -Wl,-O1 -shared -m64 -fPIC $^ -o $(TARGET) $(LIB)
-include $(OBJECTS:.o=.d)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) -MMD $(INC) -c -fPIC -o $@ $<
	@mv -f $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.d.tmp
	@sed -e 's|.*:|$(BUILD_DIR)/$*.o:|' < $(BUILD_DIR)/$*.d.tmp > $(BUILD_DIR)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILD_DIR)/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILD_DIR)/$*.d
	@rm -f $(BUILD_DIR)/$*.d.tmp

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(ASMEXT)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(INC) -c -m64 -shared -fPIC -o $@ $<

#tests
test: $(TESTOBJECTS)

$(BIN_DIR)/%: $(TEST_DIR)/%.$(SRCEXT)
	@mkdir -p $(BIN_DIR)
	$(eval HTTP := $(if $(findstring webserver,$(<)), $(HTTP_PARSER), ))
	$(CXX) -O3 -g -o $@ $(HTTP) $< -luThreads

clean:
	@echo " Cleaning..."
	find ./$(BUILD_DIR) -type f -name '*.o' -delete
	find ./$(BUILD_DIR) -type f -name '*.d' -delete
	rm -rf $(BIN_DIR)/*
	rm -rf $(LIB_DIR)/*

install: all
	mkdir -p $(DEST_DIR)/include/uThreads
	[ -d $(DEST_DIR)/lib ] || mkdir -p $(DEST_DIR)/lib
	rm -rf $(DEST_DIR)/lib/$(LIB_NAME)*
	cp lib/$(LIB_FULL_NAME) $(DEST_DIR)/lib
	ln -s $(DEST_DIR)/lib/$(LIB_FULL_NAME) $(DEST_DIR)/lib/$(LIB_NAME).$(VERSION_MAJOR)
	ln -s $(DEST_DIR)/lib/$(LIB_FULL_NAME) $(DEST_DIR)/lib/$(LIB_NAME)
	cp $(INCLUDE_DIR)/*	$(DEST_DIR)/include/uThreads
	cd $(SRC_DIR); find . -type f -name '*.h' | cpio -updm $(DEST_DIR)/include/uThreads

# Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
CC=
AR=
CP=cp
CFLAGS=
EXTRA_CFLAGS=
EXTRA_ARCHIVE_FLAGS=rc
LIB=
BAREMETAL_SRCS_DIR=./src/
INCLUDES=-I./src/include -I./src/include/common_layer/ -I./src/include/common_layer/aegothers/ -I./thirdparty/ELFIO/ -I./thirdparty/ELFIO/elfio/ -I./thirdparty/cert/handshake/ -I./thirdparty/cert/api/
BAREMETAL_CPP_SRCS=
OTHER_LIB=
DCONFIG_O_A72=
GCONFIG_O_A72=
GMIO_O_A72=
RTP_O_A72=

DCONFIG_O_R53=
GCONFIG_O_R53=
GMIO_O_R53=
RTP_O_R53=

DCONFIG_O_R52=
GCONFIG_O_R52=
GMIO_O_R52=
RTP_O_R52=

AIEMETASRC=
AIEMETAOBJ := baremetal_metadata.o
RELEASEDIR := ./build
TEMP_DIR := $(RELEASEDIR)/temp/
TARGETA72 := libaiebaremetala72.a
TARGETA78 := libaiebaremetala78.a
TARGETR53 := libaiebaremetalr53.a
TARGETR52 := libaiebaremetalr52.a
TARGET_RELEASE := $(RELEASEDIR)/libaiebaremetal_api.a
DOCS_DIR = ./docs/tmp
DOXYGEN_CONFIG_FILE = ./docs/aiebaremetal_documentation.dox

SHELL[unexport] = "1"

all: $(TARGETA72)
a78: $(TARGETA78)
rpu: $(TARGETR53)
rpu_r52: $(TARGETR52)
release: $(TARGET_RELEASE)

#BAREMETAL_SRCS := $(filter-out $(BAREMETAL_SRCS_EXCLUDE), $(wildcard $(BAREMETAL_SRCS_DIR)/*.cc)) 
#BAREMETAL_SRCS_CPP := $(filter-out $(BAREMETAL_SRCS_EXCLUDE), $(wildcard $(BAREMETAL_SRCS_DIR)/*/*.cpp)) 
BAREMETAL_SRCS_CC := $(filter-out $(BAREMETAL_SRCS_EXCLUDE), $(wildcard $(BAREMETAL_SRCS_DIR)/*.cc))
BAREMETAL_SRCS_CPP = $(filter-out $(BAREMETAL_SRCS_EXCLUDE), $(wildcard $(BAREMETAL_SRCS_DIR)/*/*.cpp))
BAREMETAL_SRCS_CPP2 = $(filter-out $(BAREMETAL_SRCS_EXCLUDE), $(wildcard $(BAREMETAL_SRCS_DIR)/*/*/*.cpp))

BAREMETAL_OBJS := $(BAREMETAL_SRCS_CC:%.cc=%.o) $(BAREMETAL_SRCS_CPP:%.cpp=%.o) $(BAREMETAL_SRCS_CPP2:%.cpp=%.o)
#OTHER_LIB
$(TARGETA72): $(AIEMETAOBJ) $(BAREMETAL_OBJS) |$(TEMP_DIR)
	rm -f $(TEMP_DIR)/*.o
	$(AR) --output $(TEMP_DIR) -x $(OTHER_LIB)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) ${RELEASEDIR}/$(TARGETA72) $(BAREMETAL_OBJS) $(TEMP_DIR)/*.o \
		$(AIEMETAOBJ)

$(TARGETA78): $(AIEMETAOBJ) $(BAREMETAL_OBJS) |$(TEMP_DIR)
	rm -f $(TEMP_DIR)/*.o
	$(AR) --output $(TEMP_DIR) -x $(OTHER_LIB)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) ${RELEASEDIR}/$(TARGETA78) $(BAREMETAL_OBJS) $(TEMP_DIR)/*.o \
		$(AIEMETAOBJ)

$(TARGET_RELEASE_AIELIB): $(AIEMETAOBJ) $(BAREMETAL_OBJS) |$(TEMP_DIR)
	rm -f $(TEMP_DIR)/*.o
	$(AR) --output $(TEMP_DIR) -x $(OTHER_LIB)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) ${RELEASEDIR}/$(TARGET_RELEASE) $(BAREMETAL_OBJS) $(TEMP_DIR)/*.o \

$(TARGET_RELEASE): $(AIEMETAOBJ) $(BAREMETAL_OBJS)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) $(TARGET_RELEASE) $(BAREMETAL_OBJS) 	

$(TARGETR53): $(AIEMETAOBJ) $(BAREMETAL_OBJS) |$(TEMP_DIR)
	rm -f $(TEMP_DIR)/*.o
	$(AR) --output $(TEMP_DIR) -x $(OTHER_LIB)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) ${RELEASEDIR}/$(TARGETR53) $(BAREMETAL_OBJS) $(TEMP_DIR)/*.o \
		$(AIEMETAOBJ)


	rm -f $(BAREMETAL_OBJS)
	#rm $(TEMP_DIR)/*.o

$(TARGETR52): $(AIEMETAOBJ) $(BAREMETAL_OBJS) |$(TEMP_DIR)
	echo "Root Makefile: target r52"
	rm -f $(TEMP_DIR)/*.o
	$(AR) --output $(TEMP_DIR) -x $(OTHER_LIB)
	$(AR) $(EXTRA_ARCHIVE_FLAGS) ${RELEASEDIR}/$(TARGETR52) $(BAREMETAL_OBJS) $(TEMP_DIR)/*.o \
		$(AIEMETAOBJ)


	rm -f $(BAREMETAL_OBJS)
	#rm $(TEMP_DIR)/*.o

$(BAREMETAL_SRCS_DIR)/%.o : $(BAREMETAL_SRCS_DIR)/%.cc |$(BAREMETAL_SRCS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BAREMETAL_SRCS_DIR)/%.o : $(BAREMETAL_SRCS_DIR)/%.cpp |$(BAREMETAL_SRCS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(AIEMETAOBJ) : $(AIEMETASRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEMP_DIR):
	@mkdir -p $(@D)

doc-generate:
	mkdir -p $(DOCS_DIR)
	doxygen $(DOXYGEN_CONFIG_FILE)
	cd $(DOCS_DIR)/latex; make; cd ..

doc-clean:
	rm -rf $(DOCS_DIR)

clean:
	rm ./build/*.a
	rm ./src/*.o
	rm ./src/common_layer/*.o

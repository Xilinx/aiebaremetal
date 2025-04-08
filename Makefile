# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

CC=
AR=
CP=cp
CFLAGS=
EXTRA_CFLAGS=
EXTRA_ARCHIVE_FLAGS=rc
LIB=
BAREMETAL_SRCS_DIR=./src/
INCLUDES=-I./include -I./include/common_layer/ -I./include/common_layer/abrothers/
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
AIEMETASRC=
AIEMETAOBJ := baremetal_metadata.o
RELEASEDIR := ./build
TEMP_DIR := $(RELEASEDIR)/temp/
TARGETA72 := libaiebaremetala72.a
TARGETR53 := libaiebaremetalr53.a
TARGET_RELEASE := $(RELEASEDIR)/libaiebaremetal_api.a

SHELL[unexport] = "1"

all: $(TARGETA72)
rpu: $(TARGETR53)
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

$(BAREMETAL_SRCS_DIR)/%.o : $(BAREMETAL_SRCS_DIR)/%.cc |$(BAREMETAL_SRCS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BAREMETAL_SRCS_DIR)/%.o : $(BAREMETAL_SRCS_DIR)/%.cpp |$(BAREMETAL_SRCS_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(AIEMETAOBJ) : $(AIEMETASRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEMP_DIR):
	@mkdir -p $(@D)

clean:
	rm ./build/*.a
	rm ./src/*.o
	rm ./src/common_layer/*.o

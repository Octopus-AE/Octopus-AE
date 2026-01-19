CC = gcc
LINK = gcc
DPUCC = dpu-upmem-dpurte-clang
DPULINK = dpu-upmem-dpurte-clang

DPU_DIR := dpu
HOST_DIR := host
BUILD_DIR := bin
OBJ_DIR := obj
INC_DIR := include

NR_DPUS ?= 64
NR_TASKLETS ?= 16
GRAPH ?= CP
PATTERN ?= HYP3_4_5

COMMON_CCFLAGS := -c -Wall -Wextra -g -O2 -I${INC_DIR} -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -D${GRAPH} -DDPU_BINARY=\"${BUILD_DIR}/dpu\" -DDPU_ALLOC_BINARY=\"${BUILD_DIR}/dpu_alloc\" -D${PATTERN}
HOST_CCFLAGS := ${COMMON_CCFLAGS} -std=c11 `dpu-pkg-config --cflags dpu` 
DPU_CCFLAGS := ${COMMON_CCFLAGS}
COMMON_LFLAGS := -DNR_TASKLETS=${NR_TASKLETS}
HOST_LFLAGS := ${COMMON_LFLAGS} `dpu-pkg-config --libs dpu`
DPU_LFLAGS := ${COMMON_LFLAGS}

INC_FILE := ${INC_DIR}/common.h ${INC_DIR}/cyclecount.h ${INC_DIR}/timer.h ${INC_DIR}/dpu_mine.h

.PHONY: all all_before host dpu clean test test_single test_all

all: all_before ${BUILD_DIR}/host ${BUILD_DIR}/dpu 

all_before:
	@mkdir -p ${BUILD_DIR}
	@mkdir -p ${OBJ_DIR}
	@mkdir -p ${OBJ_DIR}/${HOST_DIR}
	@mkdir -p ${OBJ_DIR}/${DPU_DIR}
	@mkdir -p result

${BUILD_DIR}/host: ${OBJ_DIR}/${HOST_DIR}/main.o ${OBJ_DIR}/${HOST_DIR}/partition.o 
	@${LINK} $^ ${HOST_LFLAGS}  -o $@

${BUILD_DIR}/dpu: ${OBJ_DIR}/${DPU_DIR}/main.o ${OBJ_DIR}/${DPU_DIR}/set_op.o ${OBJ_DIR}/${DPU_DIR}/${PATTERN}.o
	@${DPULINK} ${DPU_LFLAGS} $^ -o $@

${OBJ_DIR}/${HOST_DIR}/%.o: ${HOST_DIR}/%.c ${INC_FILE}
	@${CC} ${HOST_CCFLAGS} $< -o $@

${OBJ_DIR}/${DPU_DIR}/%.o: ${DPU_DIR}/%.c ${INC_FILE}
	@${DPUCC} ${DPU_CCFLAGS} $< -o $@

clean:
	@rm -rf ${BUILD_DIR} ${OBJ_DIR}

test:
	@make clean --no-print-directory
	@make all --no-print-directory
	@./${BUILD_DIR}/host

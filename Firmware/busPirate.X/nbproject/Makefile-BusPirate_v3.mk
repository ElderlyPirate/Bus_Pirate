#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-BusPirate_v3.mk)" "nbproject/Makefile-local-BusPirate_v3.mk"
include nbproject/Makefile-local-BusPirate_v3.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=BusPirate_v3
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS
SUB_IMAGE_ADDRESS_COMMAND=--image-address $(SUB_IMAGE_ADDRESS)
else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../1wire.c ../base.c ../basic.c ../bitbang.c ../dio.c ../jtag.c ../jtag/lenval.c ../jtag/micro.c ../jtag/ports.c ../main.c ../pc_at_keyboard.c ../pic.c ../raw2wire.c ../raw3wire.c ../selftest.c ../smps.c ../sump.c ../i2c.c ../hd44780.c ../spi.c ../uart.c ../openocd.c ../openocd_asm.s ../messages_v3.s ../messages.c ../binary_io.c ../proc_menu.c ../core.c ../uart2.c ../aux_pin.c ../base_io.c ../raw_common.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1472/1wire.o ${OBJECTDIR}/_ext/1472/base.o ${OBJECTDIR}/_ext/1472/basic.o ${OBJECTDIR}/_ext/1472/bitbang.o ${OBJECTDIR}/_ext/1472/dio.o ${OBJECTDIR}/_ext/1472/jtag.o ${OBJECTDIR}/_ext/760884769/lenval.o ${OBJECTDIR}/_ext/760884769/micro.o ${OBJECTDIR}/_ext/760884769/ports.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o ${OBJECTDIR}/_ext/1472/pic.o ${OBJECTDIR}/_ext/1472/raw2wire.o ${OBJECTDIR}/_ext/1472/raw3wire.o ${OBJECTDIR}/_ext/1472/selftest.o ${OBJECTDIR}/_ext/1472/smps.o ${OBJECTDIR}/_ext/1472/sump.o ${OBJECTDIR}/_ext/1472/i2c.o ${OBJECTDIR}/_ext/1472/hd44780.o ${OBJECTDIR}/_ext/1472/spi.o ${OBJECTDIR}/_ext/1472/uart.o ${OBJECTDIR}/_ext/1472/openocd.o ${OBJECTDIR}/_ext/1472/openocd_asm.o ${OBJECTDIR}/_ext/1472/messages_v3.o ${OBJECTDIR}/_ext/1472/messages.o ${OBJECTDIR}/_ext/1472/binary_io.o ${OBJECTDIR}/_ext/1472/proc_menu.o ${OBJECTDIR}/_ext/1472/core.o ${OBJECTDIR}/_ext/1472/uart2.o ${OBJECTDIR}/_ext/1472/aux_pin.o ${OBJECTDIR}/_ext/1472/base_io.o ${OBJECTDIR}/_ext/1472/raw_common.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1472/1wire.o.d ${OBJECTDIR}/_ext/1472/base.o.d ${OBJECTDIR}/_ext/1472/basic.o.d ${OBJECTDIR}/_ext/1472/bitbang.o.d ${OBJECTDIR}/_ext/1472/dio.o.d ${OBJECTDIR}/_ext/1472/jtag.o.d ${OBJECTDIR}/_ext/760884769/lenval.o.d ${OBJECTDIR}/_ext/760884769/micro.o.d ${OBJECTDIR}/_ext/760884769/ports.o.d ${OBJECTDIR}/_ext/1472/main.o.d ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d ${OBJECTDIR}/_ext/1472/pic.o.d ${OBJECTDIR}/_ext/1472/raw2wire.o.d ${OBJECTDIR}/_ext/1472/raw3wire.o.d ${OBJECTDIR}/_ext/1472/selftest.o.d ${OBJECTDIR}/_ext/1472/smps.o.d ${OBJECTDIR}/_ext/1472/sump.o.d ${OBJECTDIR}/_ext/1472/i2c.o.d ${OBJECTDIR}/_ext/1472/hd44780.o.d ${OBJECTDIR}/_ext/1472/spi.o.d ${OBJECTDIR}/_ext/1472/uart.o.d ${OBJECTDIR}/_ext/1472/openocd.o.d ${OBJECTDIR}/_ext/1472/openocd_asm.o.d ${OBJECTDIR}/_ext/1472/messages_v3.o.d ${OBJECTDIR}/_ext/1472/messages.o.d ${OBJECTDIR}/_ext/1472/binary_io.o.d ${OBJECTDIR}/_ext/1472/proc_menu.o.d ${OBJECTDIR}/_ext/1472/core.o.d ${OBJECTDIR}/_ext/1472/uart2.o.d ${OBJECTDIR}/_ext/1472/aux_pin.o.d ${OBJECTDIR}/_ext/1472/base_io.o.d ${OBJECTDIR}/_ext/1472/raw_common.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1472/1wire.o ${OBJECTDIR}/_ext/1472/base.o ${OBJECTDIR}/_ext/1472/basic.o ${OBJECTDIR}/_ext/1472/bitbang.o ${OBJECTDIR}/_ext/1472/dio.o ${OBJECTDIR}/_ext/1472/jtag.o ${OBJECTDIR}/_ext/760884769/lenval.o ${OBJECTDIR}/_ext/760884769/micro.o ${OBJECTDIR}/_ext/760884769/ports.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o ${OBJECTDIR}/_ext/1472/pic.o ${OBJECTDIR}/_ext/1472/raw2wire.o ${OBJECTDIR}/_ext/1472/raw3wire.o ${OBJECTDIR}/_ext/1472/selftest.o ${OBJECTDIR}/_ext/1472/smps.o ${OBJECTDIR}/_ext/1472/sump.o ${OBJECTDIR}/_ext/1472/i2c.o ${OBJECTDIR}/_ext/1472/hd44780.o ${OBJECTDIR}/_ext/1472/spi.o ${OBJECTDIR}/_ext/1472/uart.o ${OBJECTDIR}/_ext/1472/openocd.o ${OBJECTDIR}/_ext/1472/openocd_asm.o ${OBJECTDIR}/_ext/1472/messages_v3.o ${OBJECTDIR}/_ext/1472/messages.o ${OBJECTDIR}/_ext/1472/binary_io.o ${OBJECTDIR}/_ext/1472/proc_menu.o ${OBJECTDIR}/_ext/1472/core.o ${OBJECTDIR}/_ext/1472/uart2.o ${OBJECTDIR}/_ext/1472/aux_pin.o ${OBJECTDIR}/_ext/1472/base_io.o ${OBJECTDIR}/_ext/1472/raw_common.o

# Source Files
SOURCEFILES=../1wire.c ../base.c ../basic.c ../bitbang.c ../dio.c ../jtag.c ../jtag/lenval.c ../jtag/micro.c ../jtag/ports.c ../main.c ../pc_at_keyboard.c ../pic.c ../raw2wire.c ../raw3wire.c ../selftest.c ../smps.c ../sump.c ../i2c.c ../hd44780.c ../spi.c ../uart.c ../openocd.c ../openocd_asm.s ../messages_v3.s ../messages.c ../binary_io.c ../proc_menu.c ../core.c ../uart2.c ../aux_pin.c ../base_io.c ../raw_common.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-BusPirate_v3.mk dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24FJ64GA002
MP_LINKER_FILE_OPTION=,--script="../p24FJ64GA002.gld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/1wire.o: ../1wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/1wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/1wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../1wire.c  -o ${OBJECTDIR}/_ext/1472/1wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/1wire.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/1wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/base.o: ../base.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base.c  -o ${OBJECTDIR}/_ext/1472/base.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/base.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/base.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/basic.o: ../basic.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/basic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/basic.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../basic.c  -o ${OBJECTDIR}/_ext/1472/basic.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/basic.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/basic.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/bitbang.o: ../bitbang.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/bitbang.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/bitbang.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../bitbang.c  -o ${OBJECTDIR}/_ext/1472/bitbang.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/bitbang.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/bitbang.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/dio.o: ../dio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/dio.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/dio.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../dio.c  -o ${OBJECTDIR}/_ext/1472/dio.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/dio.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/dio.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/jtag.o: ../jtag.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag.c  -o ${OBJECTDIR}/_ext/1472/jtag.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/jtag.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/jtag.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/lenval.o: ../jtag/lenval.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/lenval.c  -o ${OBJECTDIR}/_ext/760884769/lenval.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/lenval.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/lenval.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/micro.o: ../jtag/micro.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/micro.c  -o ${OBJECTDIR}/_ext/760884769/micro.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/micro.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/micro.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/ports.o: ../jtag/ports.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/ports.c  -o ${OBJECTDIR}/_ext/760884769/ports.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/ports.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/ports.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../main.c  -o ${OBJECTDIR}/_ext/1472/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/pc_at_keyboard.o: ../pc_at_keyboard.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../pc_at_keyboard.c  -o ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/pic.o: ../pic.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/pic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/pic.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../pic.c  -o ${OBJECTDIR}/_ext/1472/pic.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/pic.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/pic.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw2wire.o: ../raw2wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw2wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw2wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw2wire.c  -o ${OBJECTDIR}/_ext/1472/raw2wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw2wire.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw2wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw3wire.o: ../raw3wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw3wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw3wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw3wire.c  -o ${OBJECTDIR}/_ext/1472/raw3wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw3wire.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw3wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/selftest.o: ../selftest.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/selftest.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/selftest.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../selftest.c  -o ${OBJECTDIR}/_ext/1472/selftest.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/selftest.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/selftest.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/smps.o: ../smps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/smps.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/smps.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../smps.c  -o ${OBJECTDIR}/_ext/1472/smps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/smps.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/smps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/sump.o: ../sump.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/sump.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/sump.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../sump.c  -o ${OBJECTDIR}/_ext/1472/sump.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/sump.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/sump.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/i2c.o: ../i2c.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/i2c.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/i2c.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../i2c.c  -o ${OBJECTDIR}/_ext/1472/i2c.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/i2c.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/i2c.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/hd44780.o: ../hd44780.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/hd44780.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/hd44780.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../hd44780.c  -o ${OBJECTDIR}/_ext/1472/hd44780.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/hd44780.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/hd44780.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/spi.o: ../spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/spi.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../spi.c  -o ${OBJECTDIR}/_ext/1472/spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/spi.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/uart.o: ../uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../uart.c  -o ${OBJECTDIR}/_ext/1472/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/uart.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/uart.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/openocd.o: ../openocd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../openocd.c  -o ${OBJECTDIR}/_ext/1472/openocd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/openocd.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/openocd.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/messages.o: ../messages.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/messages.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/messages.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../messages.c  -o ${OBJECTDIR}/_ext/1472/messages.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/messages.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/messages.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/binary_io.o: ../binary_io.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/binary_io.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/binary_io.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../binary_io.c  -o ${OBJECTDIR}/_ext/1472/binary_io.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/binary_io.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/binary_io.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/proc_menu.o: ../proc_menu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/proc_menu.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/proc_menu.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../proc_menu.c  -o ${OBJECTDIR}/_ext/1472/proc_menu.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/proc_menu.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/proc_menu.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/core.o: ../core.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/core.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/core.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../core.c  -o ${OBJECTDIR}/_ext/1472/core.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/core.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/core.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/uart2.o: ../uart2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/uart2.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/uart2.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../uart2.c  -o ${OBJECTDIR}/_ext/1472/uart2.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/uart2.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/uart2.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/aux_pin.o: ../aux_pin.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/aux_pin.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/aux_pin.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../aux_pin.c  -o ${OBJECTDIR}/_ext/1472/aux_pin.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/aux_pin.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/aux_pin.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/base_io.o: ../base_io.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base_io.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base_io.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base_io.c  -o ${OBJECTDIR}/_ext/1472/base_io.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/base_io.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/base_io.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw_common.o: ../raw_common.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw_common.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw_common.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw_common.c  -o ${OBJECTDIR}/_ext/1472/raw_common.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw_common.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw_common.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/_ext/1472/1wire.o: ../1wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/1wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/1wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../1wire.c  -o ${OBJECTDIR}/_ext/1472/1wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/1wire.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/1wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/base.o: ../base.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base.c  -o ${OBJECTDIR}/_ext/1472/base.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/base.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/base.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/basic.o: ../basic.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/basic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/basic.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../basic.c  -o ${OBJECTDIR}/_ext/1472/basic.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/basic.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/basic.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/bitbang.o: ../bitbang.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/bitbang.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/bitbang.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../bitbang.c  -o ${OBJECTDIR}/_ext/1472/bitbang.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/bitbang.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/bitbang.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/dio.o: ../dio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/dio.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/dio.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../dio.c  -o ${OBJECTDIR}/_ext/1472/dio.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/dio.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/dio.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/jtag.o: ../jtag.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag.c  -o ${OBJECTDIR}/_ext/1472/jtag.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/jtag.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/jtag.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/lenval.o: ../jtag/lenval.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/lenval.c  -o ${OBJECTDIR}/_ext/760884769/lenval.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/lenval.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/lenval.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/micro.o: ../jtag/micro.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/micro.c  -o ${OBJECTDIR}/_ext/760884769/micro.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/micro.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/micro.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/760884769/ports.o: ../jtag/ports.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/ports.c  -o ${OBJECTDIR}/_ext/760884769/ports.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/760884769/ports.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/760884769/ports.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../main.c  -o ${OBJECTDIR}/_ext/1472/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/pc_at_keyboard.o: ../pc_at_keyboard.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../pc_at_keyboard.c  -o ${OBJECTDIR}/_ext/1472/pc_at_keyboard.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/pc_at_keyboard.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/pic.o: ../pic.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/pic.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/pic.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../pic.c  -o ${OBJECTDIR}/_ext/1472/pic.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/pic.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/pic.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw2wire.o: ../raw2wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw2wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw2wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw2wire.c  -o ${OBJECTDIR}/_ext/1472/raw2wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw2wire.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw2wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw3wire.o: ../raw3wire.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw3wire.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw3wire.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw3wire.c  -o ${OBJECTDIR}/_ext/1472/raw3wire.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw3wire.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw3wire.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/selftest.o: ../selftest.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/selftest.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/selftest.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../selftest.c  -o ${OBJECTDIR}/_ext/1472/selftest.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/selftest.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/selftest.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/smps.o: ../smps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/smps.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/smps.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../smps.c  -o ${OBJECTDIR}/_ext/1472/smps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/smps.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/smps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/sump.o: ../sump.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/sump.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/sump.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../sump.c  -o ${OBJECTDIR}/_ext/1472/sump.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/sump.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/sump.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/i2c.o: ../i2c.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/i2c.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/i2c.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../i2c.c  -o ${OBJECTDIR}/_ext/1472/i2c.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/i2c.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/i2c.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/hd44780.o: ../hd44780.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/hd44780.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/hd44780.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../hd44780.c  -o ${OBJECTDIR}/_ext/1472/hd44780.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/hd44780.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/hd44780.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/spi.o: ../spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/spi.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../spi.c  -o ${OBJECTDIR}/_ext/1472/spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/spi.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/uart.o: ../uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../uart.c  -o ${OBJECTDIR}/_ext/1472/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/uart.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/uart.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/openocd.o: ../openocd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../openocd.c  -o ${OBJECTDIR}/_ext/1472/openocd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/openocd.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/openocd.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/messages.o: ../messages.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/messages.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/messages.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../messages.c  -o ${OBJECTDIR}/_ext/1472/messages.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/messages.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/messages.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/binary_io.o: ../binary_io.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/binary_io.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/binary_io.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../binary_io.c  -o ${OBJECTDIR}/_ext/1472/binary_io.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/binary_io.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/binary_io.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/proc_menu.o: ../proc_menu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/proc_menu.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/proc_menu.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../proc_menu.c  -o ${OBJECTDIR}/_ext/1472/proc_menu.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/proc_menu.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/proc_menu.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/core.o: ../core.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/core.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/core.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../core.c  -o ${OBJECTDIR}/_ext/1472/core.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/core.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/core.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/uart2.o: ../uart2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/uart2.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/uart2.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../uart2.c  -o ${OBJECTDIR}/_ext/1472/uart2.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/uart2.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/uart2.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/aux_pin.o: ../aux_pin.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/aux_pin.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/aux_pin.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../aux_pin.c  -o ${OBJECTDIR}/_ext/1472/aux_pin.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/aux_pin.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/aux_pin.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/base_io.o: ../base_io.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base_io.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base_io.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base_io.c  -o ${OBJECTDIR}/_ext/1472/base_io.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/base_io.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/base_io.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/raw_common.o: ../raw_common.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/raw_common.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/raw_common.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../raw_common.c  -o ${OBJECTDIR}/_ext/1472/raw_common.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/raw_common.o.d"        -g -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -mlarge-data -O1 -fomit-frame-pointer -I"../../microchip/include" -I".." -mcci -msmart-io=1 -Werror -Wall -msfr-warn=off -finline  -save-temps -finline
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/raw_common.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/openocd_asm.o: ../openocd_asm.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd_asm.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd_asm.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../openocd_asm.s  -o ${OBJECTDIR}/_ext/1472/openocd_asm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1  -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  -I".." -Wa,-MD,"${OBJECTDIR}/_ext/1472/openocd_asm.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,-g,--no-relax,-ahmlsi=${OBJECTDIR}/_ext/1472/openocd_asm.lst$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/openocd_asm.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/1472/messages_v3.o: ../messages_v3.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/messages_v3.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/messages_v3.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../messages_v3.s  -o ${OBJECTDIR}/_ext/1472/messages_v3.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1  -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  -I".." -Wa,-MD,"${OBJECTDIR}/_ext/1472/messages_v3.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,-g,--no-relax,-ahmlsi=${OBJECTDIR}/_ext/1472/messages_v3.lst$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/messages_v3.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/_ext/1472/openocd_asm.o: ../openocd_asm.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd_asm.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/openocd_asm.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../openocd_asm.s  -o ${OBJECTDIR}/_ext/1472/openocd_asm.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  -I".." -Wa,-MD,"${OBJECTDIR}/_ext/1472/openocd_asm.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax,-ahmlsi=${OBJECTDIR}/_ext/1472/openocd_asm.lst$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/openocd_asm.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/_ext/1472/messages_v3.o: ../messages_v3.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/messages_v3.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/messages_v3.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  ../messages_v3.s  -o ${OBJECTDIR}/_ext/1472/messages_v3.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  -I".." -Wa,-MD,"${OBJECTDIR}/_ext/1472/messages_v3.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax,-ahmlsi=${OBJECTDIR}/_ext/1472/messages_v3.lst$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/messages_v3.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    ../p24FJ64GA002.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1  -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x825 -mreserve=data@0x826:0x84F   -Wl,,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,$(MP_LINKER_FILE_OPTION),--check-sections,--data-init,--pack-data,--handles,--isr,--gc-sections,--fill-upper=0,--stackguard=16,--library-path="../../../../Program Files/Microchip/MPLAB C30/lib",--library-path="..",--no-force-link,--smart-io,-Map="BusPirate_V3.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   ../p24FJ64GA002.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_BusPirate_v3=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -Wl,,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--check-sections,--data-init,--pack-data,--handles,--isr,--gc-sections,--fill-upper=0,--stackguard=16,--library-path="../../../../Program Files/Microchip/MPLAB C30/lib",--library-path="..",--no-force-link,--smart-io,-Map="BusPirate_V3.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/busPirate.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf  
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/BusPirate_v3
	${RM} -r dist/BusPirate_v3

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif

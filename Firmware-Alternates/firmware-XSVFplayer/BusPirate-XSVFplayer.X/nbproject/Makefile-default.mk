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
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=../jtag/lenval.c ../jtag/micro.c ../jtag/ports.c ../main.c ../base.c ../jtag.c ../UART.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/760884769/lenval.o ${OBJECTDIR}/_ext/760884769/micro.o ${OBJECTDIR}/_ext/760884769/ports.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/base.o ${OBJECTDIR}/_ext/1472/jtag.o ${OBJECTDIR}/_ext/1472/UART.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/760884769/lenval.o.d ${OBJECTDIR}/_ext/760884769/micro.o.d ${OBJECTDIR}/_ext/760884769/ports.o.d ${OBJECTDIR}/_ext/1472/main.o.d ${OBJECTDIR}/_ext/1472/base.o.d ${OBJECTDIR}/_ext/1472/jtag.o.d ${OBJECTDIR}/_ext/1472/UART.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/760884769/lenval.o ${OBJECTDIR}/_ext/760884769/micro.o ${OBJECTDIR}/_ext/760884769/ports.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/base.o ${OBJECTDIR}/_ext/1472/jtag.o ${OBJECTDIR}/_ext/1472/UART.o

# Source Files
SOURCEFILES=../jtag/lenval.c ../jtag/micro.c ../jtag/ports.c ../main.c ../base.c ../jtag.c ../UART.c



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
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24FJ64GA002
MP_LINKER_FILE_OPTION=,--script="..\p24FJ64GA002.gld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/760884769/lenval.o: ../jtag/lenval.c  .generated_files/db1f6170807d7ec83455df82ea40f7b21ab6357b.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/lenval.c  -o ${OBJECTDIR}/_ext/760884769/lenval.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/lenval.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/760884769/micro.o: ../jtag/micro.c  .generated_files/8f9548b693d2652e5354316b02ef6c7657e52e50.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/micro.c  -o ${OBJECTDIR}/_ext/760884769/micro.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/micro.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/760884769/ports.o: ../jtag/ports.c  .generated_files/3a1fddb7142e374d65e497e21c5c694bd857a452.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/ports.c  -o ${OBJECTDIR}/_ext/760884769/ports.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/ports.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  .generated_files/7e27686378fb8df604e818ce4d9e38c9fa430b81.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../main.c  -o ${OBJECTDIR}/_ext/1472/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/base.o: ../base.c  .generated_files/3e84e7c4b426c183bde50d72f840a0797acceb5.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base.c  -o ${OBJECTDIR}/_ext/1472/base.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/base.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/jtag.o: ../jtag.c  .generated_files/f2f12f8cd53c76a05f6354c963098c4e50bb0fee.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag.c  -o ${OBJECTDIR}/_ext/1472/jtag.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/jtag.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/UART.o: ../UART.c  .generated_files/f97a92a8ea90b0a4d162d2f9bdc125308c419c19.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/UART.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/UART.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../UART.c  -o ${OBJECTDIR}/_ext/1472/UART.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/UART.o.d"      -g -D__DEBUG     -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/_ext/760884769/lenval.o: ../jtag/lenval.c  .generated_files/e447dd3e1bb27af1b0762414fd912595033a8b73.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/lenval.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/lenval.c  -o ${OBJECTDIR}/_ext/760884769/lenval.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/lenval.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/760884769/micro.o: ../jtag/micro.c  .generated_files/f57a11c12136bd112706d5f798c17fd18d7955b1.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/micro.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/micro.c  -o ${OBJECTDIR}/_ext/760884769/micro.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/micro.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/760884769/ports.o: ../jtag/ports.c  .generated_files/5622a19334500ecc0f96f0521d0d2978b5bda9b5.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/760884769" 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o.d 
	@${RM} ${OBJECTDIR}/_ext/760884769/ports.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag/ports.c  -o ${OBJECTDIR}/_ext/760884769/ports.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/760884769/ports.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/main.o: ../main.c  .generated_files/a77a9f7d5defaee1057cd978b4de4ae28b6501b4.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../main.c  -o ${OBJECTDIR}/_ext/1472/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/main.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/base.o: ../base.c  .generated_files/481ce0381048be58accc89cbb29b7affbb2d2f32.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/base.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../base.c  -o ${OBJECTDIR}/_ext/1472/base.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/base.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/jtag.o: ../jtag.c  .generated_files/42c42981877f26dbb5246f7ae49ed4c941a5628c.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/jtag.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../jtag.c  -o ${OBJECTDIR}/_ext/1472/jtag.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/jtag.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/_ext/1472/UART.o: ../UART.c  .generated_files/659fc04c2a8dd41aef6bad5afa294301be9080c8.flag .generated_files/63dfb0bfd128dd8b4733f68df85a51deed10f4e8.flag
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/UART.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/UART.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../UART.c  -o ${OBJECTDIR}/_ext/1472/UART.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/_ext/1472/UART.o.d"        -g -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -O0 -I".." -I"../jtag" -I"." -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    ../p24FJ64GA002.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)      -Wl,,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--library-path="..",--library-path="C:/Program Files/Microchip/MPLAB C30/lib",--library-path=".",--no-force-link,--smart-io,-Map="${DISTDIR}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   ../p24FJ64GA002.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_default=$(CND_CONF)  -no-legacy-libc  $(COMPARISON_BUILD)  -Wl,,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--library-path="..",--library-path="C:/Program Files/Microchip/MPLAB C30/lib",--library-path=".",--no-force-link,--smart-io,-Map="${DISTDIR}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	${MP_CC_DIR}\\xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/BusPirate-XSVFplayer.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf   -mdfp="${DFP_DIR}/xc16" 
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif

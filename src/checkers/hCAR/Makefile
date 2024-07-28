ifeq ($(SOLVER),MINISAT)
SOLVER_FLAG=-DMINISAT
else
SOLVER_FLAG=-DGLUCOSE
endif

ifeq ($(TRAIL),ON)
TRAIL_FLAG=-DTRAIL
endif

ifeq ($(DIVE),ON)
DIVE_FLAG=-DDIVE
endif

ifeq ($(INTER),ON)
INTER_FLAG=-DINTER
else
INTER_FLAG=
endif

ifeq ($(ROTATE),ON)
ROTATE_FLAG=-DROTATE
else
ROTATE_FLAG=
endif

ifeq ($(SCORE),ON)
SCORE_FLAG=-DSCORE
else
SCORE_FLAG=
endif

ifeq ($(SCORE_REV),ON)
SCORE_REV_FLAG=-DSCORE_REVERSE
else
SCORE_REV_FLAG=
endif

ifeq ($(SCORE_ABS),ON)
SCORE_OPTION_FLAG=-DSCORE_ABS
else
SCORE_OPTION_FLAG=
endif

ifeq ($(SCORE_DECAY),ON)
SCORE_DECAY_FLAG=-DSCORE_DECAY
else
SCORE_DECAY_FLAG=
endif


ifeq ($(CONV),ON)
CONVERGENCE_FLAG:=-DCONVERGENCE -DCONV_HEAVY
else ifeq ($(CONV), LIGHT)
CONVERGENCE_FLAG:=-DCONVERGENCE 
else
CONVERGENCE_FLAG:=
endif

CONV_LOW_THRESH ?= 0
ifneq ($(CONV_LOW_THRESH), 0)
CONVERGENCE_FLAG += -DCONV_LOW_THRESH=$(CONV_LOW_THRESH)
endif

CONV_STUCK_THRESH ?= 0
ifneq ($(CONV_STUCK_THRESH), 0)
CONVERGENCE_FLAG += -DCONV_STUCK_THRESH=$(CONV_STUCK_THRESH)
endif

CONV_STAT ?= 0
ifneq ($(CONV_STAT), 0)
CONVERGENCE_FLAG += -DCONV_STAT
endif


ifeq ($(FALLIN),ON)
FALLIN_FLAG=-DFALLIN_STATS
else
FALLIN_FLAG=
endif




INTER_CNT ?= 1
INTER_CNT_FLAG = -DINTER_CNT=${INTER_CNT}

ifeq ($(FRESHUC),ON)
FRESHUC_FLAG=-DFRESH_UC
else
FRESHUC_FLAG=
endif

ifeq ($(FRESHU),ON)
FRESH_U_FLAG=-DFRESH_U
else
FRESH_U_FLAG=
endif

ifeq ($(ORDERU),ON)
ORDERED_U_FLAG=-DORDERED_U
else
ORDERED_U_FLAG=
endif

ifeq ($(CONTAINER),PQUEUE)
CONTAINER_FLAG=-DPQUEUE
else
CONTAINER_FLAG=-DSTACK
endif


ifeq ($(PARTIAL),ON)
PARTIAL_FLAG=-DPARTIAL
else
PARTIAL_FLAG=
endif

# this is much too heavy, which is disastrous as to unsafe bench.
ifeq ($(INV_LEVEL),HIGH)
INV_FLAG=-DINV_HEAVY
else ifeq ($(INV_LEVEL),MEDIUM)
INV_FLAG=-DINV_MEDIUM
else ifeq ($(INV_LEVEL),LOW)
INV_FLAG=-DINV_LIGHT
else
INV_FLAG=
endif

ifeq ($(SIMPU_LEVEL),HIGH)
SIMPU_FLAG=-DSIMPU_HEAVY
SIMPU_THRESH ?= 16
SIMPU_THRESH_FLAG=-DSIMPU_THRESH=${SIMPU_THRESH}
# else ifeq ($(SIMPU_LEVEL),MEDIUM)
# SIMPU_FLAG=-DINV_MEDIUM
else ifeq ($(SIMPU_LEVEL),LOW)
SIMPU_FLAG=-DSIMPU_LIGHT
SIMPU_THRESH ?= 16
SIMPU_THRESH_FLAG=-DSIMPU_THRESH=${SIMPU_THRESH}
else
SIMPU_FLAG=
endif


ifeq ($(SIMPU_INC),ON)
SIMPU_INC_FLAG=-DSIMPU_INC
else
SIMPU_INC_FLAG=
endif



SEED ?= 0

ifeq ($(SEED),0)
RANDOM_FLAG= 
else
RANDOM_FLAG=-DRANDSEED=$(SEED)
endif

ifeq ($(PICK),RANDOM)
PICK_FLAG=-DRANDOM_PICK
else
PICK_FLAG=
endif

ifeq ($(ASSU),IRRI)
ASSUM_FLAG=-DASS_IRRI
else ifeq ($(ASSU),IIRR)
ASSUM_FLAG=-DASS_IIRR
else ifeq ($(ASSU),IRIR)
ASSUM_FLAG=-DASS_IRIR
else ifeq ($(ASSU),RIRI)
ASSUM_FLAG=-DASS_RIRI
else ifeq ($(ASSU),RRII)
ASSUM_FLAG=-DASS_RRII
else ifeq ($(ASSU),RIIR)
ASSUM_FLAG=-DASS_RIIR
else ifeq ($(ASSU),SHUFFLE_INTER)
ASSUM_FLAG=-DASS_SHUFFLE_INTER
else
ASSUM_FLAG=
endif

ifeq ($(INTER_RAND),ON)
INTER_RAND_FLAG=-DINTER_RAND
else
INTER_RAND_FLAG= 
endif

ifeq ($(INTER_LONG),ON)
INTER_LONG_FLAG=-DINTER_LONG
else
INTER_LONG_FLAG= 
endif

ifeq ($(INTER_SHORT),ON)
INTER_SHORT_FLAG=-DINTER_SHORT
else
INTER_SHORT_FLAG= 
endif

ifeq ($(INTER_RES_LONG), ON)
INTER_RES_LONG_FLAG=-DINTER_RES_LONG
else
INTER_RES_LONG_FLAG=
endif

ifeq ($(INTER_ORDER),LAST_FIRST)
INTER_ORDER_FLAG=-DLAST_FIRST
else ifeq ($(INTER_ORDER),REVERSE)
INTER_ORDER_FLAG=-DINTER_REVERSE
else ifeq ($(INTER_ORDER),SHUFFLE)
INTER_ORDER_FLAG=-DINTER_SHUFFLE
else ifeq ($(INTER_ORDER),LF_SHUFFLE)
INTER_ORDER_FLAG=-DINTER_LF_SHUFFLE
else
INTER_ORDER_FLAG=
endif

ifeq ($(INTER_INVALIDATE),SIMPLE)
INTER_INVALIDATE_FLAG=-DINTER_INVALIDATE_SIMPLE
else ifeq ($(INTER_INVALIDATE),HARD)
INTER_INVALIDATE_FLAG=-DINTER_INVALIDATE_HARD
else
INTER_INVALIDATE_FLAG= 
endif

ifeq ($(FRONT),ON)
FRONT_FLAG=-DFRONT_FLAG
else
FRONT_FLAG=
endif

ifeq ($(PICK_GUIDE), RANDOM)
GUIDE_FLAG=-DPICK_GUIDE_RANDOM
else
GUIDE_FLAG=
endif

PP_CNT ?=-1
ifeq ($(PP_CNT), -1)
PP_LIMIT_CNT_FLAG=
else
PP_LIMIT_CNT_FLAG=-DPP_LIMIT_CNT=$(PP_CNT)
endif

PP_TIME ?= -1
ifeq ($(PP_TIME), -1)
PP_LIMIT_TIME_FLAG=
else
PP_LIMIT_TIME_FLAG=-DPP_LIMIT_TIME=$(PP_TIME)
endif

MAXNI ?= -1
ifeq ($(MAXNI), -1)
MAXNI_FLAG=
else
MAXNI_FLAG=-DMAXNI=$(MAXNI)
endif

COOK ?= -1
ifeq ($(COOK), -1)
COOK_FLAG=
else
COOK_FLAG=-DCOOK=$(COOK)
endif

ifeq ($(COOK_LIGHT), ON)
COOK_LIGHT_FLAG=-DCOOK_LIGHT
else
COOK_LIGHT_FLAG=
endif

ifeq ($(INCOMPLETE), ON)
INCOMPLETE_FLAG=-DINCOMPLETE
else
INCOMPLETE_FLAG=
endif

ifeq ($(PP_MEM), ON)
PP_MEM_FLAG=-DPP_MEM
else
PP_MEM_FLAG=
endif

ifeq ($(SOLVER),MINISAT)
SOURCES = model/aiger.c model/model.cpp\
			checker/carChecker.cpp checker/bmcChecker.cpp \
			solver/carsolver.cpp solver/mainsolver.cpp solver/newpartialsolver.cpp \
			utils/data_structure.cpp utils/utility.cpp \
			newmain.cpp \
			sat/minisat/core/Solver.cc sat/minisat/utils/Options.cc sat/minisat/utils/System.cc		
INCLUDE_DIRS = -I./src -I./model -I./checker -I./debug -I./sat -I./solver -I./utils -I./sat/minisat -I./sat/minisat/core -I./sat/minisat/utils
CFLAG = $(INCLUDE_DIRS) -D__STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -c -g
else
SOURCES = model/aiger.c model/model.cpp\
			checker/carChecker.cpp checker/bmcChecker.cpp \
			solver/carsolver.cpp solver/mainsolver.cpp solver/newpartialsolver.cpp \
			utils/data_structure.cpp utils/utility.cpp \
			newmain.cpp \
			sat/glucose/core/Solver.cc sat/glucose/utils/Options.cc sat/glucose/utils/System.cc
INCLUDE_DIRS = -I./src -I./model -I./checker -I./debug -I./sat -I./solver -I./utils -I./sat/glucose -I./sat/glucose/core -I./sat/glucose/utils
CFLAG = $(INCLUDE_DIRS) -D__STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -c -g
endif

OBJECTS = carsolver.o newpartialsolver.o mainsolver.o model.o newmain.o utility.o data_structure.o aiger.o\
	Solver.o Options.o System.o carChecker.o bmcChecker.o

# consider using -fprofile-generate and -fprofile-use
OPTFLAG = -Ofast -march=native -frename-registers -funroll-loops -fno-signed-zeros

LFLAG = -g -lz -lpthread 

GCC = gcc

GXX = g++

TARGET ?= caramel

OPTIONS=$(TRAIL_FLAG) ${INTER_FLAG} ${ROTATE_FLAG} ${RANDOM_FLAG} ${CONTAINER_FLAG} ${FRESHUC_FLAG} ${FRESH_U_FLAG} ${ORDERED_U_FLAG} ${PARTIAL_FLAG} ${INV_FLAG} ${SIMPU_FLAG} ${SIMPU_THRESH_FLAG} ${SIMPU_INC_FLAG} ${PICK_FLAG} ${INTER_RAND_FLAG}  ${ASSUM_FLAG} ${DIVE_FLAG} ${INTER_LONG_FLAG} ${INTER_SHORT_FLAG} ${GUIDE_FLAG} ${FALLIN_FLAG} ${SOLVER_FLAG} ${INTER_INVALIDATE_FLAG} ${INTER_ORDER_FLAG} ${PP_LIMIT_CNT_FLAG} ${PP_LIMIT_TIME_FLAG} ${MAXNI_FLAG} ${COOK_FLAG} ${COOK_LIGHT_FLAG} ${INTER_CNT_FLAG} ${INCOMPLETE_FLAG} $(PP_MEM_FLAG) $(INTER_RES_LONG_FLAG) $(SCORE_FLAG) $(SCORE_OPTION_FLAG) $(SCORE_REV_FLAG) $(FRONT_FLAG) $(CONVERGENCE_FLAG) $(SCORE_DECAY_FLAG)

caramel:$(SOURCES)
	$(GCC) ${OPTFLAG} ${OPTIONS} $(CFLAG) $(SOURCES)
	$(GXX) ${OPTFLAG} -g -o ${TARGET} $(OBJECTS) $(LFLAG)

clean: 
	rm *.o
	rm caramel

.PHONY: caramel

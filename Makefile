#### Mini CAR ####
MCAR_DIR = src/checkers/miniCAR

MCAR: 
	$(MAKE) -C $(MCAR_DIR) TARGET=MCAR
	mkdir -p bin
	cp $(MCAR_DIR)/MCAR bin



#### simple_CAR ####

SIMPLECAR_DIR = simple_CAR

SIMPLECAR:
	$(MAKE) -C $(SIMPLECAR_DIR)
	mkdir -p bin
	cp $(SIMPLECAR_DIR)/simplecar bin


SUBDIRS = $(MCAR_DIR) $(SIMPLECAR_DIR) # To be added here
all: MCAR SIMPLECAR # To be added here

	
clean:
	rm -f bin/*
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean
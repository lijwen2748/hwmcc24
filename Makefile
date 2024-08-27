#### Mini CAR ####
MCAR_DIR = src/checkers/MCAR

MCAR: 
	$(MAKE) -C $(MCAR_DIR) TARGET=MCAR
	cp $(MCAR_DIR)/MCAR bin




SUBDIRS = $(MCAR_DIR) # To be added here
all: MCAR # To be added here

	
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean
#### Hybrid CAR ####
HCAR_DIR = src/checkers/hCAR

HCAR: 
	$(MAKE) -C $(HCAR_DIR) TARGET=HCAR
	cp $(HCAR_DIR)/HCAR bin


SUBDIRS = $(HCAR_DIR) # To be added here
all: HCAR # To be added here

	
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean
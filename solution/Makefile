BINS = wfs mkfs
CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=gnu18 -g
FUSE_CFLAGS = `pkg-config fuse --cflags --libs`

# Default to no debug
DEBUG ?= 0

# Add debug flag conditionally
ifeq ($(DEBUG),1)
    CFLAGS += -DDEBUG
endif


.PHONY: all
all: $(BINS)

wfs:
	$(CC) $(CFLAGS) wfs.c $(FUSE_CFLAGS) -o wfs
mkfs:
	$(CC) $(CFLAGS) -o mkfs mkfs.c

.PHONY: clean
clean:
	rm -rf $(BINS)


# .PHONY: cbwfs
# cbwfs: clean wfs

# .PHONY: cbmkfs
# cbmkfs: clean mkfs

.PHONY: cbwfs
cbwfs: clean
	$(MAKE) wfs CFLAGS="$(CFLAGS) -DDEBUG"

.PHONY: cbmkfs
cbmkfs: clean
	$(MAKE) mkfs CFLAGS="$(CFLAGS) -DDEBUG"

.PHONY: cba
cba: clean 
	$(MAKE) mkfs CFLAGS="$(CFLAGS) -DDEBUG"
	$(MAKE) wfs CFLAGS="$(CFLAGS) -DDEBUG"

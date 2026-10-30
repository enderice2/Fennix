NAME=gcc
OBJECT_NAME=lib$(NAME).a
OUTPUT_DIR=$(WORKSPACE_DIR)/out/lib/
CRTBEGIN_PATH = $(shell $(CC) -print-file-name=libgcc.a)

build: $(OBJECT_NAME)

$(OBJECT_NAME):
	cp $(CRTBEGIN_PATH) $(OUTPUT_DIR)

clean:

CC?=gcc
UPLOADER-SRC= \
	Utils/Encoding.c \
	Utils/ShaSum.c \
	Utils/sha1.c \
	B2Upload.c \
	$(NULL)

OBF-SOURCE= \
	stringObs.c
all: $(UPLOADER-SRC)
	$(CC) $(UPLOADER-SRC) -lcurl -lpthread -o B2Uploader

release: $(UPLOADER-SRC)
	$(CC) $(UPLOADER-SRC) -lcurl -lpthread -DRELEASE -o B2Uploader


.phony: Obfuscator release

Obfuscator: stringObs.c
	$(CC) stringObs.c -o Obfuscator

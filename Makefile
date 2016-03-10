CC=gcc
CFLAGS=-std=c99 -Wall -fPIC -shared -ldl -lutil -lpam #-DDEBUG
OUTNAME=h0h0

all: $(OUTNAME).so

$(OUTNAME).so:
	@echo "[+] Compiling: $(OUTNAME).c -> $(OUTNAME).so"
	$(CC) $(CFLAGS) -o $(OUTNAME).so $(OUTNAME).c
	@echo

clean:
	@echo '[+] Cleaning up...'
	rm -f $(OUTNAME).so
	@echo

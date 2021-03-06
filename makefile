CC=clang

list = PNG.o macPaint.o

macPaint_PNG: $(list)
	$(CC) -o macPaint_PNG -lz $(list)

# ====
PNG.o: PNG.c
	$(CC) -c PNG.c

macPaint.o: macPaint.c macPaint.h
	$(CC) -c macPaint.c

# ==== CLEAN
clean:	
	rm $(list)

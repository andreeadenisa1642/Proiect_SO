# fisierul makefile pentru compilarea și rularea programului mai rapida

# Makefile pentru compilarea și rularea programului
program: program.c
	gcc -o program program.c

run: program
	./program img

clean:
	rm -f program


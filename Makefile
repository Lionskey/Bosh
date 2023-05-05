bosh: sh.c
	gcc sh.c -o bosh -lreadline
install:
	cp bosh /bin/

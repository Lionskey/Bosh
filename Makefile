bosh: sh.c
	gcc sh.c -o bosh -lreadline
install:
	install -c bosh /bin/
install-man:
	install -c man/bosh.1.gz /usr/share/man/man1/


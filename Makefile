# Makefile
pwcheck:
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -o proj2
clean:
	rm proj2
install:
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -o proj2

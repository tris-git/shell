all:
    gcc main.c -o main -Wall -Werror -fsanitize=address,leak

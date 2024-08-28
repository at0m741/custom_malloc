NAME = custom_alloc
CC = clang

CFLAGS = -g -mavx2

SRC = $(wildcard *.c)

OBJ = $(SRC:.c=.o)

ifeq ($(DEBUG), true)
	CFLAGS += -D DEBUG
endif

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

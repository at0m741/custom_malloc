NAME = custom_alloc
SO_NAME = ./libft_malloc_x86_64_Linux.so
CC = clang

CFLAGS = -g -O3 -mavx2 -fPIC  -mprefer-vector-width=256# -fPIC est nécessaire pour les bibliothèques partagées

SRC = $(wildcard *.c)

OBJ_DIR = objs
OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)

ifeq ($(DEBUG), true)
	CFLAGS += -D DEBUG
endif

all: $(OBJ_DIR) $(NAME) $(SO_NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

$(SO_NAME): $(OBJ)
	$(CC) -shared -o $(SO_NAME) $(OBJ)

clean:
	rm -f $(OBJ_DIR)/*.o

fclean: clean
	rm -rf $(OBJ_DIR)
	rm -f $(NAME) $(SO_NAME)

re: fclean all

.PHONY: all clean fclean re

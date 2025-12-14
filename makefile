# variables
COMPLIER = c++
DELETE   = rm -rf
NAME     = webserver
CPPFLAGS = -Wextra -Wall -Werror -std=c++98
HEADERS  = -I./.server/lib
OBJECTS  = ./.server/lib/extern.o \
		   ./.server/main/main.o \
		   ./.server/parse/parse.o \
		   ./.server/parse/json.o

# rules
all : $(NAME)

$(NAME) : $(OBJECTS)
	$(COMPLIER) $(HEADERS) $(CPPFLAGS) $(OBJECTS) -o $(NAME)

%.o : %.cpp
	$(COMPLIER) $(HEADERS) $(CPPFLAGS) $< -c -o $@

clean :
	$(DELETE) $(OBJECTS)

fclean : clean
	$(DELETE) $(NAME)

re : fclean all

.SECONDARY : $(OBJECTS)
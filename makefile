# variables
COMPLIER = c++
DELETE   = rm -rf
NAME     = webserver
CPPFLAGS = -Wextra -Wall -Werror -std=c++98
HEADERS  = -I./.server/console \
					 -I./.server/main \
					 -I./.server/clock \
					 -I./.server/parsing/parseArgument
OBJECTS  = ./.server/console/console.o \
					 ./.server/main/main.o \
					 ./.server/clock/clock.o \
					 ./.server/parsing/parseArgument/parseArgument.o \
					 ./.server/parsing/parseArgument/autoConfig.o \
					 ./.server/parsing/parseArgument/pathConfig.o

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
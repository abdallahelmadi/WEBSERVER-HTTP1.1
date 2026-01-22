# variables
COMPLIER = c++
DELETE   = rm -rf
NAME     = webserver
CPPFLAGS = -Wextra -Wall -Werror -std=c++98
HEADERS  = -I./.server/lib
OBJECTS  = ./.server/lib/.o \
		   ./.server/main/.o \
		   ./.server/parse/parse.o \
		   ./.server/parse/json.o \
		   ./.server/parse/generate.o \
		   ./.server/run/.o \
		   ./.server/run/utils/.o \
		   ./.server/run/method/get.o \
		   ./.server/run/method/post.o \
		   ./.server/run/method/delete.o \
		   ./.server/run/epoll_handle.o

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

tclean : fclean
	clear

re : fclean all

.SECONDARY : $(OBJECTS)
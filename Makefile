CXX	 	= 	g++
SRC		=	main.cpp \
			FreeMemory.cpp
CXXFLAGS	=	-W -Wall -Werror -Wextra -std=c++11
OBJS		=	$(SRC:.cpp=.o)
NAME		=	sixfree

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME) $(LIB)

clean:
	rm -f $(OBJS)
	rm -f *~ \#*

fclean:	clean
	rm -f $(NAME)

re:	fclean all

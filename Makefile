CXX	 	= 	g++
SRC		=	main.cpp \
			FreeMemory.cpp
CXXFLAGS	=	-W -Wall -Werror -Wextra -std=c++11 -DBOOST_LOG_DYN_LINK 
OBJS		=	$(SRC:.cpp=.o)
NAME		=	sixfree

LDFLAGS		= 	-lboost_thread -lboost_system -lboost_log_setup -lboost_log -lpthread

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME) $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f *~ \#*

fclean:	clean
	rm -f $(NAME)

re:	fclean all

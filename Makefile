CXX	 	= 	g++
CXXFLAGS	=	-W -Wall -Werror -Wextra -std=c++11 -DBOOST_LOG_DYN_LINK
LDFLAGS		= 	-lboost_thread -lboost_system -lboost_log_setup -lboost_log -lboost_filesystem -lboost_program_options -lpthread
OBJS		=	$(SRC:.cpp=.o)
SRC		=	main.cpp \
			FreeMemory.cpp
NAME		=	sixfree

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME) $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f *~ \#*

fclean:	clean
	rm -f $(NAME)

re:	fclean all

NAME = ircserv

CXX = c++
#CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I. -Ihandlers -Iparser

SRC = server.cpp \
	handlers/CommandProcessor.cpp \
	handlers/Dispatcher.cpp \
	handlers/Handlers.cpp \
	handlers/SendHelpers.cpp \
	handlers/ServerState.cpp \
	handlers/State.cpp \
	parser/Message.cpp \
	parser/Parser.cpp

OBJ_DIR = obj
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

RM = rm -rf

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o ircserv

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re

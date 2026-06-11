# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: skomyshe <skomyshe@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/20 18:36:56 by skomyshe          #+#    #+#              #
#    Updated: 2026/06/09 20:57:08 by skomyshe         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserve
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17

SRCS = CGIExecutor.cpp CGIEnv.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re party
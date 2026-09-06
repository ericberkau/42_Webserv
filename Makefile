# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/11 14:54:40 by eberkau           #+#    #+#              #
#    Updated: 2026/09/06 18:23:22 by eberkau          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		=	webserv

CXX			=	c++
ERRFLAGS	=	-Wall -Wextra -Werror
CXXFLAGS	=	$(ERRFLAGS) -DNDEBUG
SHADOW		=	-Wshadow
VERSFLAG	=	-std=c++17
INCLUDE		=	-I$(INCLDIR)

RM			=	rm -fr

INCLDIR		=	include
SRCDIR		=	srcs
OBJDIR		=	.objs

SRCS		=	$(addprefix $(SRCDIR)/, $(CXXFILES))
OBJS		=	$(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS		=	$(OBJS:.o=.d)

GREEN		=	\033[0;32m
RED			=	\033[0;31m
BLUE		=	\033[0;34m
DEFAULT		=	\033[0m

# list source files here
CXXFILES	=	testmain.cpp \
				configValidation.cpp \
				signals.cpp \
				classes/Client.class.cpp \
				classes/Lexer.class.cpp \
				classes/Parser.core.class.cpp \
				classes/Parser.location.class.cpp \
				classes/Parser.server.class.cpp \
				classes/parserUtils.cpp \
				classes/Server.class.cpp \
				classes/HttpRequestContainer.cpp \
				classes/RequestParser.cpp \
				classes/RequestHandler.cpp \
				classes/ResponseBuilder.cpp \
#				and_so_on.cpp


all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Building executable binary file '$(NAME)' ...$(DEFAULT)"
	@$(CXX) $(CXXFLAGS) $(SHADOW) $(VERSFLAG) $^ -o $@
	@echo "$(BLUE)------- '$(NAME)' compiled successfully! -------$(DEFAULT)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SHADOW) $(VERSFLAG) $(INCLUDE) -MMD -MP -c $< -o $@

$(OBJDIR):
	@mkdir $(OBJDIR)
	@echo "$(GREEN)Building object (.o) files ...$(DEFAULT)"

clean:
	@$(RM) $(OBJDIR)
	@echo "$(RED)Removed object (.o) files$(DEFAULT)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(RED)Removed executable binary file '$(NAME)'$(DEFAULT)"

re: re_msg fclean all

re_msg:
	@echo "$(BLUE)Rebuilding '$(NAME)' ...$(DEFAULT)"

debug: CXXFLAGS = $(ERRFLAGS) -g -O0 -fsanitize=address
debug: re
	@echo "$(RED)ATTENTION:$(BLUE) Built in $(RED)DEBUG$(BLUE) mode!$(DEFAULT)"

-include $(DEPS)

.PHONY: all clean fclean re re_msg debug

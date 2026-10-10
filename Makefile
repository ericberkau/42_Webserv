# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/11 14:54:40 by eberkau           #+#    #+#              #
#    Updated: 2026/10/10 03:02:59 by eberkau          ###   ########.fr        #
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
				classes/client/Client.class.cpp \
				classes/config/Lexer.class.cpp \
				classes/config/Parser.core.class.cpp \
				classes/config/Parser.location.class.cpp \
				classes/config/Parser.server.class.cpp \
				classes/config/parserUtils.cpp \
				classes/HTTP/HttpRequestContainer.class.cpp \
				classes/HTTP/RequestHandler.class.cpp \
				classes/HTTP/RequestParser.class.cpp \
				classes/HTTP/ResponseBuilder.class.cpp \
				classes/HTTP/StringUtils.cpp \
				classes/server/Server.class.cpp \
				classes/server/Server.cgi.class.cpp \
				classes/server/Server.client.class.cpp \
				classes/server/Server.setup.class.cpp \
				classes/CGI/CGIEnv.class.cpp \
				classes/CGI/CGIExecutor.class.cpp \
				configValidation.cpp \
				signals.cpp \
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

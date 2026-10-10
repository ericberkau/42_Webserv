# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eberkau <eberkau@student.42heilbronn.de    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/11 14:54:40 by eberkau           #+#    #+#              #
#    Updated: 2026/10/10 22:14:45 by eberkau          ###   ########.fr        #
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

RED			=	\033[0;31m
GREEN		=	\033[0;32m
YELLOW      =	\033[0;33m
BLUE		=	\033[0;34m
MAGENTA     =	\033[0;35m
CYAN        =	\033[0;36m
DEFAULT		=	\033[0m

# list source files here
CXXFILES	=	main.cpp \
				classes/client/Client.class.cpp \
				classes/config/ConfigLexer.class.cpp \
				classes/config/ConfigParser.core.class.cpp \
				classes/config/ConfigParser.location.class.cpp \
				classes/config/ConfigParser.server.class.cpp \
				classes/config/configParserUtils.cpp \
				classes/HTTP/HttpRequestContainer.class.cpp \
				classes/HTTP/RequestHandler.class.cpp \
				classes/HTTP/RequestParser.class.cpp \
				classes/HTTP/ResponseBuilder.class.cpp \
				classes/server/Server.class.cpp \
				classes/server/Server.cgi.class.cpp \
				classes/server/Server.client.class.cpp \
				classes/server/Server.setup.class.cpp \
				classes/CGI/CGIEnv.class.cpp \
				classes/CGI/CGIExecutor.class.cpp \
				configPrinting.cpp \
				configValidation.cpp \
				signals.cpp \
				stringUtils.cpp \
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

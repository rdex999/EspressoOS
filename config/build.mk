# 
# This file is part of the EspressoOS project (https://github.com/rdex999/EspressoOS.git).
# Copyright (c) 2025 David Weizman.
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
# 
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

export SRC:=source
export BLD:=build
export CC:=g++
export AS:=nasm
export LD:=ld
export CFLAGS+=-m64 -c -ffreestanding -Wall -Wextra \
	-fno-stack-protector -fno-exceptions -fno-rtti 	\
	-I $(SRC)/include -I libk/include
export ASFLAGS+=-f elf64 -I $(SRC)

export TEXT_END:=$(shell tput sgr0)
export TEXT_BOLD:=$(shell tput bold)

export TEXT_CYAN:=$(shell tput setaf 6)
export TEXT_YELLOW:=$(shell tput setaf 3)
export TEXT_GREEN:=$(shell tput setaf 2)

# Adds attributes into a string, for example changing the color, making it in bold, etc..
# PARAMETERS
#	1 => The string itself.
# 	2 => The first formating option (optional)
# 	3 => The second formating option (optional)
define text_attr = 
$(2)$(3)$(1)$(TEXT_END)
endef

# Prints the "Compiling <SOURCE_FILE> into <TARGET_FILE>..." message, 
# and creates the directory tree in the build folder for <TARGET_FILE>.
# PARAMETERS
# 	1 => The target file (typically the .obj file)
# 	2 => The source file
define prep_compile =
	$(eval $@_TARGET = $(patsubst $(BLD)/%,%,$(1)))
	$(eval $@_SOURCE = $(patsubst $(SRC)/%,%,$(2)))
	@echo -e \
		"$(call text_attr,Compiling,$(TEXT_BOLD),$(TEXT_CYAN)) \
		$(call text_attr,${$@_SOURCE},$(TEXT_GREEN)) \
		into \
		$(call text_attr,${$@_TARGET},$(TEXT_YELLOW))..."
	@mkdir -p $(dir $(1))
endef
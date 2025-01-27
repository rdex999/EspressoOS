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

export SRC=source
export BLD=build
export CC=g++
export AS=nasm
export LD=ld
export CFLAGS+=-m64 -c -ffreestanding -Wall -Wextra \
	-fno-stack-protector -fno-exceptions -fno-rtti 	\
	-I $(SRC)/include -I libk/include
export ASFLAGS+=-f elf64 -I $(SRC)
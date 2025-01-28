#!/bin/bash

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


# This file is meant to be executed only by the main makefile.

# This file compiles a directory in source/, which is given by the first parameter.

SUB_DIR=$1

if [ -f $SUB_DIR/makefile ]
then
	DIR_BLD=$BLD/$(basename $SUB_DIR)
	DIR_SRC=$SRC/$(basename $SUB_DIR)
	mkdir -p $DIR_BLD
	make \
		--no-print-directory \
		-C $SUB_DIR \
		BLD=$DIR_BLD \
		SRC=$DIR_SRC

else
	C_FILES=$(find $SUB_DIR -name *.c)
	for FILE in $C_FILES
	do
		BASE_FILE=$(basename $FILE)

		# The backslash (\) is for using the forward slash (/). 
		# Meaning, search for "source/" and replace with "build/". (Not replace "source" with "build")
		BLD_DIR=${FILE/$SRC\//$BLD\/}
		BLD_DIR=${BLD_DIR/\/$BASE_FILE/""}

		OBJECT=$BLD_DIR/${BASE_FILE/".c"/".obj"}

		# -nt Will return true if $FILE (the source file) is newer than $OBJECT, OR, if $OBJECT doesnt exist.
		# Basically what make checks on rules.
		if [ $FILE -nt $OBJECT ]
		then
			mkdir -p $BLD_DIR
			echo -e "Compiling $FILE into $OBJECT..."	
			$CC $CFLAGS -o $OBJECT $FILE
		fi
	done
fi
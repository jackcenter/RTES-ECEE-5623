# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/rpi/RTES-ECEE-5623/midterm

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/rpi/RTES-ECEE-5623/midterm/build

# Include any dependencies generated for this target.
include CMakeFiles/q_24.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/q_24.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/q_24.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/q_24.dir/flags.make

CMakeFiles/q_24.dir/question-24.c.o: CMakeFiles/q_24.dir/flags.make
CMakeFiles/q_24.dir/question-24.c.o: /home/rpi/RTES-ECEE-5623/midterm/question-24.c
CMakeFiles/q_24.dir/question-24.c.o: CMakeFiles/q_24.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/rpi/RTES-ECEE-5623/midterm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/q_24.dir/question-24.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/q_24.dir/question-24.c.o -MF CMakeFiles/q_24.dir/question-24.c.o.d -o CMakeFiles/q_24.dir/question-24.c.o -c /home/rpi/RTES-ECEE-5623/midterm/question-24.c

CMakeFiles/q_24.dir/question-24.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/q_24.dir/question-24.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/rpi/RTES-ECEE-5623/midterm/question-24.c > CMakeFiles/q_24.dir/question-24.c.i

CMakeFiles/q_24.dir/question-24.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/q_24.dir/question-24.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/rpi/RTES-ECEE-5623/midterm/question-24.c -o CMakeFiles/q_24.dir/question-24.c.s

# Object files for target q_24
q_24_OBJECTS = \
"CMakeFiles/q_24.dir/question-24.c.o"

# External object files for target q_24
q_24_EXTERNAL_OBJECTS =

q_24: CMakeFiles/q_24.dir/question-24.c.o
q_24: CMakeFiles/q_24.dir/build.make
q_24: CMakeFiles/q_24.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/rpi/RTES-ECEE-5623/midterm/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable q_24"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/q_24.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/q_24.dir/build: q_24
.PHONY : CMakeFiles/q_24.dir/build

CMakeFiles/q_24.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/q_24.dir/cmake_clean.cmake
.PHONY : CMakeFiles/q_24.dir/clean

CMakeFiles/q_24.dir/depend:
	cd /home/rpi/RTES-ECEE-5623/midterm/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/rpi/RTES-ECEE-5623/midterm /home/rpi/RTES-ECEE-5623/midterm /home/rpi/RTES-ECEE-5623/midterm/build /home/rpi/RTES-ECEE-5623/midterm/build /home/rpi/RTES-ECEE-5623/midterm/build/CMakeFiles/q_24.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/q_24.dir/depend


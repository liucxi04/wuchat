# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/liucxi/Documents/chat_luwu

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/liucxi/Documents/chat_luwu/build

# Include any dependencies generated for this target.
include luwu/CMakeFiles/test_daemon.dir/depend.make

# Include the progress variables for this target.
include luwu/CMakeFiles/test_daemon.dir/progress.make

# Include the compile flags for this target's objects.
include luwu/CMakeFiles/test_daemon.dir/flags.make

luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o: luwu/CMakeFiles/test_daemon.dir/flags.make
luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o: ../luwu/tests/test_daemon.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/liucxi/Documents/chat_luwu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o"
	cd /home/liucxi/Documents/chat_luwu/build/luwu && /usr/bin/g++  $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o -c /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp

luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i"
	cd /home/liucxi/Documents/chat_luwu/build/luwu && /usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp > CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i

luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s"
	cd /home/liucxi/Documents/chat_luwu/build/luwu && /usr/bin/g++ $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp -o CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s

# Object files for target test_daemon
test_daemon_OBJECTS = \
"CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o"

# External object files for target test_daemon
test_daemon_EXTERNAL_OBJECTS =

../luwu/bin/test_daemon: luwu/CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o
../luwu/bin/test_daemon: luwu/CMakeFiles/test_daemon.dir/build.make
../luwu/bin/test_daemon: ../luwu/lib/libluwu.so
../luwu/bin/test_daemon: /usr/lib/x86_64-linux-gnu/libssl.so
../luwu/bin/test_daemon: /usr/lib/x86_64-linux-gnu/libcrypto.so
../luwu/bin/test_daemon: luwu/CMakeFiles/test_daemon.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/liucxi/Documents/chat_luwu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../luwu/bin/test_daemon"
	cd /home/liucxi/Documents/chat_luwu/build/luwu && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_daemon.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
luwu/CMakeFiles/test_daemon.dir/build: ../luwu/bin/test_daemon

.PHONY : luwu/CMakeFiles/test_daemon.dir/build

luwu/CMakeFiles/test_daemon.dir/clean:
	cd /home/liucxi/Documents/chat_luwu/build/luwu && $(CMAKE_COMMAND) -P CMakeFiles/test_daemon.dir/cmake_clean.cmake
.PHONY : luwu/CMakeFiles/test_daemon.dir/clean

luwu/CMakeFiles/test_daemon.dir/depend:
	cd /home/liucxi/Documents/chat_luwu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/liucxi/Documents/chat_luwu /home/liucxi/Documents/chat_luwu/luwu /home/liucxi/Documents/chat_luwu/build /home/liucxi/Documents/chat_luwu/build/luwu /home/liucxi/Documents/chat_luwu/build/luwu/CMakeFiles/test_daemon.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : luwu/CMakeFiles/test_daemon.dir/depend


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
CMAKE_SOURCE_DIR = /home/liucxi/Documents/chat_luwu/luwu

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/liucxi/Documents/chat_luwu/luwu/build

# Include any dependencies generated for this target.
include CMakeFiles/test_daemon.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/test_daemon.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_daemon.dir/flags.make

CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o: CMakeFiles/test_daemon.dir/flags.make
CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o: ../tests/test_daemon.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/liucxi/Documents/chat_luwu/luwu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o -c /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp

CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp > CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.i

CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_daemon.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/liucxi/Documents/chat_luwu/luwu/tests/test_daemon.cpp -o CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.s

# Object files for target test_daemon
test_daemon_OBJECTS = \
"CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o"

# External object files for target test_daemon
test_daemon_EXTERNAL_OBJECTS =

../bin/test_daemon: CMakeFiles/test_daemon.dir/tests/test_daemon.cpp.o
../bin/test_daemon: CMakeFiles/test_daemon.dir/build.make
../bin/test_daemon: ../lib/libluwu.so
../bin/test_daemon: /usr/lib/x86_64-linux-gnu/libssl.so
../bin/test_daemon: /usr/lib/x86_64-linux-gnu/libcrypto.so
../bin/test_daemon: CMakeFiles/test_daemon.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/liucxi/Documents/chat_luwu/luwu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_daemon"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_daemon.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_daemon.dir/build: ../bin/test_daemon

.PHONY : CMakeFiles/test_daemon.dir/build

CMakeFiles/test_daemon.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_daemon.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_daemon.dir/clean

CMakeFiles/test_daemon.dir/depend:
	cd /home/liucxi/Documents/chat_luwu/luwu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/liucxi/Documents/chat_luwu/luwu /home/liucxi/Documents/chat_luwu/luwu /home/liucxi/Documents/chat_luwu/luwu/build /home/liucxi/Documents/chat_luwu/luwu/build /home/liucxi/Documents/chat_luwu/luwu/build/CMakeFiles/test_daemon.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_daemon.dir/depend


# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

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
CMAKE_SOURCE_DIR = /home/sunmmer/httpfunc/http_parser/httpfunction

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sunmmer/httpfunc/http_parser/build

# Include any dependencies generated for this target.
include src/CMakeFiles/decoder.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/decoder.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/decoder.dir/flags.make

src/CMakeFiles/decoder.dir/Handle.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/Handle.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/Handle.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/Handle.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/Handle.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/Handle.cpp

src/CMakeFiles/decoder.dir/Handle.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/Handle.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/Handle.cpp > CMakeFiles/decoder.dir/Handle.cpp.i

src/CMakeFiles/decoder.dir/Handle.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/Handle.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/Handle.cpp -o CMakeFiles/decoder.dir/Handle.cpp.s

src/CMakeFiles/decoder.dir/Handle.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/Handle.cpp.o.requires

src/CMakeFiles/decoder.dir/Handle.cpp.o.provides: src/CMakeFiles/decoder.dir/Handle.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/Handle.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/Handle.cpp.o.provides

src/CMakeFiles/decoder.dir/Handle.cpp.o.provides.build: src/CMakeFiles/decoder.dir/Handle.cpp.o

src/CMakeFiles/decoder.dir/FormatPacket.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/FormatPacket.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/FormatPacket.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/FormatPacket.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/FormatPacket.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/FormatPacket.cpp

src/CMakeFiles/decoder.dir/FormatPacket.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/FormatPacket.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/FormatPacket.cpp > CMakeFiles/decoder.dir/FormatPacket.cpp.i

src/CMakeFiles/decoder.dir/FormatPacket.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/FormatPacket.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/FormatPacket.cpp -o CMakeFiles/decoder.dir/FormatPacket.cpp.s

src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.requires

src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.provides: src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.provides

src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.provides.build: src/CMakeFiles/decoder.dir/FormatPacket.cpp.o

src/CMakeFiles/decoder.dir/Receiver.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/Receiver.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/Receiver.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_3)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/Receiver.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/Receiver.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/Receiver.cpp

src/CMakeFiles/decoder.dir/Receiver.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/Receiver.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/Receiver.cpp > CMakeFiles/decoder.dir/Receiver.cpp.i

src/CMakeFiles/decoder.dir/Receiver.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/Receiver.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/Receiver.cpp -o CMakeFiles/decoder.dir/Receiver.cpp.s

src/CMakeFiles/decoder.dir/Receiver.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/Receiver.cpp.o.requires

src/CMakeFiles/decoder.dir/Receiver.cpp.o.provides: src/CMakeFiles/decoder.dir/Receiver.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/Receiver.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/Receiver.cpp.o.provides

src/CMakeFiles/decoder.dir/Receiver.cpp.o.provides.build: src/CMakeFiles/decoder.dir/Receiver.cpp.o

src/CMakeFiles/decoder.dir/HttpParse.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/HttpParse.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/HttpParse.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_4)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/HttpParse.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/HttpParse.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/HttpParse.cpp

src/CMakeFiles/decoder.dir/HttpParse.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/HttpParse.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/HttpParse.cpp > CMakeFiles/decoder.dir/HttpParse.cpp.i

src/CMakeFiles/decoder.dir/HttpParse.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/HttpParse.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/HttpParse.cpp -o CMakeFiles/decoder.dir/HttpParse.cpp.s

src/CMakeFiles/decoder.dir/HttpParse.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/HttpParse.cpp.o.requires

src/CMakeFiles/decoder.dir/HttpParse.cpp.o.provides: src/CMakeFiles/decoder.dir/HttpParse.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/HttpParse.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/HttpParse.cpp.o.provides

src/CMakeFiles/decoder.dir/HttpParse.cpp.o.provides.build: src/CMakeFiles/decoder.dir/HttpParse.cpp.o

src/CMakeFiles/decoder.dir/AppMain.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/AppMain.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/AppMain.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_5)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/AppMain.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/AppMain.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/AppMain.cpp

src/CMakeFiles/decoder.dir/AppMain.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/AppMain.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/AppMain.cpp > CMakeFiles/decoder.dir/AppMain.cpp.i

src/CMakeFiles/decoder.dir/AppMain.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/AppMain.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/AppMain.cpp -o CMakeFiles/decoder.dir/AppMain.cpp.s

src/CMakeFiles/decoder.dir/AppMain.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/AppMain.cpp.o.requires

src/CMakeFiles/decoder.dir/AppMain.cpp.o.provides: src/CMakeFiles/decoder.dir/AppMain.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/AppMain.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/AppMain.cpp.o.provides

src/CMakeFiles/decoder.dir/AppMain.cpp.o.provides.build: src/CMakeFiles/decoder.dir/AppMain.cpp.o

src/CMakeFiles/decoder.dir/SessionHash.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/SessionHash.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/SessionHash.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_6)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/SessionHash.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/SessionHash.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/SessionHash.cpp

src/CMakeFiles/decoder.dir/SessionHash.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/SessionHash.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/SessionHash.cpp > CMakeFiles/decoder.dir/SessionHash.cpp.i

src/CMakeFiles/decoder.dir/SessionHash.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/SessionHash.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/SessionHash.cpp -o CMakeFiles/decoder.dir/SessionHash.cpp.s

src/CMakeFiles/decoder.dir/SessionHash.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/SessionHash.cpp.o.requires

src/CMakeFiles/decoder.dir/SessionHash.cpp.o.provides: src/CMakeFiles/decoder.dir/SessionHash.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/SessionHash.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/SessionHash.cpp.o.provides

src/CMakeFiles/decoder.dir/SessionHash.cpp.o.provides.build: src/CMakeFiles/decoder.dir/SessionHash.cpp.o

src/CMakeFiles/decoder.dir/Buffer.cpp.o: src/CMakeFiles/decoder.dir/flags.make
src/CMakeFiles/decoder.dir/Buffer.cpp.o: /home/sunmmer/httpfunc/http_parser/httpfunction/src/Buffer.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sunmmer/httpfunc/http_parser/build/CMakeFiles $(CMAKE_PROGRESS_7)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/CMakeFiles/decoder.dir/Buffer.cpp.o"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/decoder.dir/Buffer.cpp.o -c /home/sunmmer/httpfunc/http_parser/httpfunction/src/Buffer.cpp

src/CMakeFiles/decoder.dir/Buffer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/decoder.dir/Buffer.cpp.i"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/sunmmer/httpfunc/http_parser/httpfunction/src/Buffer.cpp > CMakeFiles/decoder.dir/Buffer.cpp.i

src/CMakeFiles/decoder.dir/Buffer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/decoder.dir/Buffer.cpp.s"
	cd /home/sunmmer/httpfunc/http_parser/build/src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/sunmmer/httpfunc/http_parser/httpfunction/src/Buffer.cpp -o CMakeFiles/decoder.dir/Buffer.cpp.s

src/CMakeFiles/decoder.dir/Buffer.cpp.o.requires:
.PHONY : src/CMakeFiles/decoder.dir/Buffer.cpp.o.requires

src/CMakeFiles/decoder.dir/Buffer.cpp.o.provides: src/CMakeFiles/decoder.dir/Buffer.cpp.o.requires
	$(MAKE) -f src/CMakeFiles/decoder.dir/build.make src/CMakeFiles/decoder.dir/Buffer.cpp.o.provides.build
.PHONY : src/CMakeFiles/decoder.dir/Buffer.cpp.o.provides

src/CMakeFiles/decoder.dir/Buffer.cpp.o.provides.build: src/CMakeFiles/decoder.dir/Buffer.cpp.o

# Object files for target decoder
decoder_OBJECTS = \
"CMakeFiles/decoder.dir/Handle.cpp.o" \
"CMakeFiles/decoder.dir/FormatPacket.cpp.o" \
"CMakeFiles/decoder.dir/Receiver.cpp.o" \
"CMakeFiles/decoder.dir/HttpParse.cpp.o" \
"CMakeFiles/decoder.dir/AppMain.cpp.o" \
"CMakeFiles/decoder.dir/SessionHash.cpp.o" \
"CMakeFiles/decoder.dir/Buffer.cpp.o"

# External object files for target decoder
decoder_EXTERNAL_OBJECTS =

bin/decoder: src/CMakeFiles/decoder.dir/Handle.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/FormatPacket.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/Receiver.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/HttpParse.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/AppMain.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/SessionHash.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/Buffer.cpp.o
bin/decoder: src/CMakeFiles/decoder.dir/build.make
bin/decoder: src/CMakeFiles/decoder.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable ../bin/decoder"
	cd /home/sunmmer/httpfunc/http_parser/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/decoder.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/decoder.dir/build: bin/decoder
.PHONY : src/CMakeFiles/decoder.dir/build

src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/Handle.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/FormatPacket.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/Receiver.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/HttpParse.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/AppMain.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/SessionHash.cpp.o.requires
src/CMakeFiles/decoder.dir/requires: src/CMakeFiles/decoder.dir/Buffer.cpp.o.requires
.PHONY : src/CMakeFiles/decoder.dir/requires

src/CMakeFiles/decoder.dir/clean:
	cd /home/sunmmer/httpfunc/http_parser/build/src && $(CMAKE_COMMAND) -P CMakeFiles/decoder.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/decoder.dir/clean

src/CMakeFiles/decoder.dir/depend:
	cd /home/sunmmer/httpfunc/http_parser/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sunmmer/httpfunc/http_parser/httpfunction /home/sunmmer/httpfunc/http_parser/httpfunction/src /home/sunmmer/httpfunc/http_parser/build /home/sunmmer/httpfunc/http_parser/build/src /home/sunmmer/httpfunc/http_parser/build/src/CMakeFiles/decoder.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/decoder.dir/depend


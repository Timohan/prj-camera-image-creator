#/*!
#* \file Makefile
#* \brief Makefile for compiling
#*
#* Copyright of Timo hannukkala, Inc. All rights reserved.
#*
#* \author Timo Hannukkala <timohannukkala@hotmail.com>
#*/
TARGET:=prj-camera-image-creator
CXX:=$(if $(CXX),$(CXX),g++)
PKGFLAGS=`pkg-config --cflags --libs prj-tg-ui-lib`
CXXFLAGS+=-g -Wall -pedantic -c -pipe -std=gnu++17 -W -D_REENTRANT -fPIC
CXXFLAGS+=-I./src
CXXFLAGS+=$(PKGFLAGS)
#CXXFLAGS+=-Wno-unused-parameter -Wuninitialized -Wconversion -Wshadow -Wpointer-arith \
#	 -Wswitch-default -Wswitch-enum -Wcast-align \
#	 -Winline -Wundef -Wcast-qual -Wunreachable-code -Wlogical-op -Wfloat-equal \
#	 -Wredundant-decls -Werror \
#	 -Wno-unused-const-variable
LDFLAGS:=$(PKGFLAGS) -lGL -lGLEW -lglfw
# set current make dir
CURRENT_DIR=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CXXFLAGS+=-DIMAGES_PATH=\"$(CURRENT_DIR)images/\"

src_SRCDIR:=$(CURRENT_DIR)src
src_SRCS:=$(wildcard $(src_SRCDIR)/*.cpp)
src_OBJS:=$(src_SRCS:.cpp=.o)

shader_SRCDIR:=$(CURRENT_DIR)src/shader
shader_SRCS:=$(wildcard $(shader_SRCDIR)/*.cpp)
shader_OBJS:=$(shader_SRCS:.cpp=.o)

math_SRCDIR:=$(CURRENT_DIR)src/math
math_SRCS:=$(wildcard $(math_SRCDIR)/*.cpp)
math_OBJS:=$(math_SRCS:.cpp=.o)

scene_SRCDIR:=$(CURRENT_DIR)src/scene
scene_SRCS:=$(wildcard $(scene_SRCDIR)/*.cpp)
scene_OBJS:=$(scene_SRCS:.cpp=.o)

all: default

default: $(src_OBJS) $(shader_OBJS) $(math_OBJS) $(scene_OBJS)
	$(CXX) $(src_OBJS) $(shader_OBJS) $(math_OBJS) $(scene_OBJS) $(LDFLAGS) -o $(TARGET)

$(src_OBJS):%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(shader_OBJS):%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(math_OBJS):%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(scene_OBJS):%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -f src/*.o
	rm -f src/shader/*.o
	rm -f src/math/*.o
	rm -f src/scene/*.o

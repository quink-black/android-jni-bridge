JAVA_HOME ?= $(shell /usr/libexec/java_home -v 1.7)

PLATFORM = $(shell echo $(shell uname) | tr '[:upper:]' '[:lower:]')

OBJDIR    := build

CPPFLAGS  := ${CPPFLAGS} -Iinclude -I${JAVA_HOME}/include -I${JAVA_HOME}/include/${PLATFORM} -std=c++11 -Wall -Werror
CXXFLAGS  := ${CXXFLAGS} -fno-rtti -fno-exceptions -g

ifeq (${PLATFORM}, darwin)
LDFLAGS   := ${LDFLAGS} -L${OBJDIR} -Ldarwin -ljdk-7-jni -ljvm -L${JAVA_HOME}/jre/lib/server -Wl,-rpath,${JAVA_HOME}/jre/lib/server
else
LDFLAGS   := ${LDFLAGS} -Llinux -ljdk-jni -L${JAVA_HOME}/jre/lib/amd64/server -ljvm -lpthread -Wl,-rpath,${JAVA_HOME}/jre/lib/amd64/server
endif

SRCS      := $(wildcard *.cpp)
OBJS      := $(addprefix $(OBJDIR)/,$(SRCS:%.cpp=%.o))

all: ${OBJDIR}/test
	${OBJDIR}/test

${OBJDIR}/test: ${OBJS}
	$(CXX) -o $@ $^ ${LDFLAGS}

${OBJDIR}/%.o: %.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

${OBJS}: | ${OBJDIR}
     
${OBJDIR}:
	mkdir -p ${OBJDIR}

clean:
	rm -fr ${OBJDIR}

CCC = g++
CCFLAGS = -std=c++11 -Wall -Wextra
LIBS = -lSDL2
FILES = *.cc
NAME = audiosynth

all: audiosynth

run : $(NAME)
	./$(NAME)

audiosynth :
	$(CCC) $(CCFLAGS) $(FILES) -o $(NAME) $(LIBS)

clean :
	rm $(NAME)

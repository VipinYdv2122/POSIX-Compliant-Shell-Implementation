
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LIBS = -lreadline

TARGET = my_shell
SRC = q1.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

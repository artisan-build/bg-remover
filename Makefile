CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui
OPENCV_FLAGS = `pkg-config --cflags opencv4` $(OPENCV_LIBS)
TARGET = bg-remover
SOURCE = bg-remover.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) $(OPENCV_FLAGS)
	strip $(TARGET)

clean:
	rm -f $(TARGET)
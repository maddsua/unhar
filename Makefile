TARGET		= unhar.exe
OBJECTS		= main_gui.o inc/binbase64.o resource.res

$(TARGET): $(OBJECTS)
	g++ $(OBJECTS) -lole32 -luuid -lgdi32 -o $(TARGET)

main_gui.o: main_gui.cpp
	g++ -c main_gui.cpp -o main_gui.o

inc/binbase64.o: inc/binbase64.cpp
	g++ -c inc/binbase64.cpp -o inc/binbase64.o

resource.res: resource.rc
	windres -i resource.rc --input-format=rc -o resource.res -O coff

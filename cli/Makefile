TARGET		= unharcli.exe
OBJECTS		= main.o madbase64.o

$(TARGET): $(OBJECTS)
	gcc $(OBJECTS) -lole32 -luuid -lgdi32 -o $(TARGET)

main.o: main.c
	gcc -c main.c -o main.o

madbase64.o: madbase64.c
	gcc -c madbase64.c -o madbase64.o

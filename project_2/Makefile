SOURCE	:= Main.c Trace.c Branch_Predictor.c
CC	:= gcc
TARGET	:= Main
LINK	:= -lm

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -g -o $(TARGET) $(SOURCE) $(LINK)

clean:
	rm -f $(TARGET)

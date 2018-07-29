all: sample2D

sample2D: Sample_GL3_2D.cpp
	g++ -g -o sample2D Sample_GL3_2D.cpp -pthread -lglfw -lGLEW -lGL -ldl -lao

clean:
	rm sample2D

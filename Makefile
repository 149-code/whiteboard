all:
	cc main.c tablet.c operation.c -lglfw -lglew -framework OpenGl -framework ApplicationServices -fsanitize=address

all:
	cc main.c operation.c cmd.c -lglfw -lglew -framework OpenGl -framework ApplicationServices

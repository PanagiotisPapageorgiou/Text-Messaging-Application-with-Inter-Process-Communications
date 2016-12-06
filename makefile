all: setup app1 app2 enc1 enc2 chan

setup: setup.c setup_tools.c shared_tools.c shared_structs.c
	gcc -o setup setup.c setup_tools.c shared_tools.c shared_structs.c -lpthread

app1: app.c app_tools.c shared_tools.c shared_structs.c
	gcc -o app1 app.c app_tools.c shared_tools.c shared_structs.c -lpthread

app2: app.c app_tools.c shared_tools.c shared_structs.c
	gcc -o app2 app.c app_tools.c shared_tools.c shared_structs.c -lpthread

enc1: enc.c enc_tools.c shared_tools.c shared_structs.c
	gcc -o enc1 enc.c enc_tools.c shared_tools.c shared_structs.c -lpthread -lssl -lcrypto

enc2: enc.c enc_tools.c shared_tools.c shared_structs.c
	gcc -o enc2 enc.c enc_tools.c shared_tools.c shared_structs.c -lpthread -lssl -lcrypto

chan: chan.c chan_tools.c shared_tools.c shared_structs.c
	gcc -o chan chan.c chan_tools.c shared_tools.c shared_structs.c -lpthread

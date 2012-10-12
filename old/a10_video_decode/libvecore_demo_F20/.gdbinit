#filename: .gdbinit
#gdb will read it when starting
file ve
target remote 192.168.200.229:1234
#set args hello
b main
c


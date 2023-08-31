
.PHONY:all
all:exe_cgi mysql_cgi

exe_cgi:cgi.cc 
	g++ -o $@ $^ -std=c++11
mysql_cgi:mysql_connect.cc
	g++ -o $@ $^ -std=c++11 -I include -L lib -lmysqlclient -lpthread -ldl -static

.PHONY:clean
clean:
	rm -rf exe_cgi mysql_cgi
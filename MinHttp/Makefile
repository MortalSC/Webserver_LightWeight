bin=httpserver						#生成的目标文件
cgi=exe_cgi
cc=g++								#编译选择
LD_FLAGS=-std=c++11 -lpthread		#链接
src=main.cpp
curr=$(shell pwd)

.PHONY:ALL
ALL:$(bin) CGI

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

# $(cgi):cgi/cgi.cc
# 	$(cc) -o $@ $^ -std=c++11

# 数据库引入后的更新
CGI:
	cd $(curr)/cgi;\
	make;\
	cd -;

.PHONY:clean
clean:
	rm -f $(bin) 
	rm -rf output
	cd $(curr)/cgi;\
	make clean;\
	cd -;

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp ./cgi/exe_cgi output/wwwroot
	cp ./cgi/shell_cgi.sh output/wwwroot
	cp ./cgi/python_cgi.py output/wwwroot
	cp ./cgi/mysql_cgi output/wwwroot





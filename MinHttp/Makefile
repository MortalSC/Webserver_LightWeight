bin=httpserver						#生成的目标文件
cgi=exe_cgi
cc=g++								#编译选择
LD_FLAGS=-std=c++11 -lpthread		#链接
src=main.cpp
curr=$(shell pwd)

ALL:$(bin) $(cgi)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):cgi/cgi.cc
	$(cc) -o $@ $^ 

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi) output/wwwroot

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output



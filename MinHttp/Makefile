bin=httpserver						#生成的目标文件
cc=g++								#编译选择
LD_FLAGS=-std=c++11 -lpthread		#链接
src=main.cpp

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin)
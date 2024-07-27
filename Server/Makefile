EXE = Server.exe
Dir = . src osapi tcpserver jsoncpp/src utility ftp

DXX_Source = $(foreach dir, $(Dir), $(wildcard $(dir)/*.cpp))
DXX_OBJECT = $(patsubst %.cpp, %.o, $(DXX_Source))
DXX_File = $(patsubst %.o, %.d, $(DXX_OBJECT))

DXX_FLAG = -lpthread

-include $(DXX_File)

defalut: $(DXX_OBJECT)
	g++ $^ -o $(EXE) $(DXX_FLAG)

%.o: %.cpp
	g++ -std=c++11 -c $< -MMD -g -o $@

clean:
	rm -rf $(EXE) $(DXX_OBJECT) $(DXX_File)

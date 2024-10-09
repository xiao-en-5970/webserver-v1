CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif
git:
	@git add .
	@git commit -m "make and commit"
	@git push origin master
server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp git
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient
	

clean:
	rm  -r server

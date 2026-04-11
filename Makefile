TERARKDBROOT = /home/vondele/chess/noob/terarkdb
CDBDIRECTROOT = /home/vondele/chess/vondele/cdbdirect
CHESSDB_PATH = /mnt/ssd/chess-20251115/data/
CHESSDB_PATH = /media/ssd_t7/chessdb/chess-20251115/data
TERARKDBROOT = ../terarkdb
CDBDIRECTROOT = ../cdbdirect

LDFLAGS = -L$(TERARKDBROOT)/output/lib -L$(CDBDIRECTROOT)
LIBS = -lcdbdirect -lterarkdb -lterark-zip-r -lboost_fiber -lboost_context -pthread -lgcc -lrt -ldl -ltbb -laio -lgomp -lsnappy -llz4 -lz -lbz2

CXXFLAGS = -std=c++20 -march=native -flto=auto -DCHESSDB_PATH=\"$(CHESSDB_PATH)\"
ifdef DEBUG
  CXXFLAGS += -O0 -g -UNDEBUG
else
  CXXFLAGS += -O3 -g -DNDEBUG -fomit-frame-pointer -finline
endif

HEADERS = cdbshorts.h gameprogress.hpp
SOURCES = puzzles.cpp unseen.cpp fakeleaves.cpp books.cpp longpv.cpp shortpv.cpp edgy.cpp minply.cpp
BINARIES = $(SOURCES:.cpp=)

all: $(BINARIES)

%: %.cpp $(HEADERS)
	g++ $(CXXFLAGS) -I$(CDBDIRECTROOT) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(BINARIES)

format:
	clang-format -i $(SOURCES) $(HEADERS)

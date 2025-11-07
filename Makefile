TERARKDBROOT = /home/vondele/chess/noob/terarkdb
CDBDIRECTROOT = /home/vondele/chess/vondele/cdbdirect
CHESSDB_PATH = /mnt/ssd/chess-20250608/data/
CHESSDB_PATH = /media/ssd_t7/chessdb/chess-20250608/data
TERARKDBROOT = ../terarkdb
CDBDIRECTROOT = ../cdbdirect

LDFLAGS = -L$(TERARKDBROOT)/output/lib -L$(CDBDIRECTROOT)
LIBS = -lcdbdirect -lterarkdb -lterark-zip-r -lboost_fiber -lboost_context -ltcmalloc -pthread -lgcc -lrt -ldl -ltbb -laio -lgomp -lsnappy -llz4 -lz -lbz2
CXXFLAGS = -std=c++20 -O3 -march=native -fomit-frame-pointer -finline -flto=auto
CXXFLAGS += -DCHESSDB_PATH=\"$(CHESSDB_PATH)\"

SOURCES = puzzles.cpp unseen.cpp fakeleaves.cpp books.cpp longpv.cpp shortpv.cpp
BINARIES = $(SOURCES:.cpp=)

all: $(BINARIES)

%: %.cpp
	g++ $(CXXFLAGS) -I$(CDBDIRECTROOT) -o $@ $< $(LDFLAGS) $(LIBS)

clean:
	rm -f $(BINARIES)

format:
	clang-format -i $(SOURCES)

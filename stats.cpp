#include "cdbdirect.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "external/chess.hpp"
using namespace chess;

template <typename T>
struct StatsTemplate {
  T count_total{0}, has_ply{0}, ply{0}, eval{0}, mobility{0}, piececount{0};
};

using AtomicStats = StatsTemplate<std::atomic<uint64_t>>;
using Stats = StatsTemplate<uint64_t>;

Stats snapshot(const AtomicStats& source) {
  Stats s;
  s.count_total = source.count_total.load(std::memory_order_relaxed);
  s.has_ply     = source.has_ply.load(std::memory_order_relaxed);
  s.ply         = source.ply.load(std::memory_order_relaxed);
  s.eval        = source.eval.load(std::memory_order_relaxed);
  s.mobility    = source.mobility.load(std::memory_order_relaxed);
  s.piececount  = source.piececount.load(std::memory_order_relaxed);
  return s;
}

int main(int argc, char *argv[]) {
  std::uintptr_t handle = cdbdirect_initialize(CHESSDB_PATH);
  std::uint64_t db_size = cdbdirect_size(handle);
  std::cout << "DB count: " << db_size << std::endl;
  std::cout << "index  has_ply  ply  eval  pc  mob" << std::endl;

  const size_t batch_size = 10000000;

  AtomicStats global;
  Stats last_printed;
  std::mutex print_mutex;

  auto evaluate_entry =
    [&](const std::string &fen,
        const std::vector<std::pair<std::string, int>> &scored) {
      
      Board board(fen);
      Movelist moves;
      movegen::legalmoves(moves, board);
      
      int eval = std::min(std::abs(scored.front().second), 200);
      int ply = scored.back().second;
      if (ply >= 0) {
        global.has_ply.fetch_add(1, std::memory_order_relaxed);
        global.ply.fetch_add(ply, std::memory_order_relaxed);
      }
      global.eval.fetch_add(eval, std::memory_order_relaxed);
      global.mobility.fetch_add(moves.size(), std::memory_order_relaxed);
      global.piececount.fetch_add(board.occ().count(), std::memory_order_relaxed);

      uint64_t peek = global.count_total.fetch_add(1, std::memory_order_relaxed);

      if ((peek + 1) % batch_size == 0 || peek == db_size) {
        std::lock_guard<std::mutex> lock(print_mutex);
        Stats curr = snapshot(global);
        auto d_total = curr.count_total - last_printed.count_total;
        
        auto d_ply_c = curr.has_ply - last_printed.has_ply;
        auto d_ply_s = curr.ply - last_printed.ply;
        auto d_eval  = curr.eval - last_printed.eval;
        auto d_mob   = curr.mobility - last_printed.mobility;
        auto d_pcs   = curr.piececount - last_printed.piececount;

        std::cout << curr.count_total << " " 
                  << (d_ply_c * 100.0 / d_total) << " " 
                  << (d_ply_c ? d_ply_s / (double) d_ply_c : 0.0) << " " 
                  << d_eval / (double) d_total << " "
                  << d_pcs / (double) d_total << " "
                  << d_mob / (double) d_total <<  std::endl;

        last_printed = curr;
      }

      return peek < db_size;
    };

  const size_t num_threads = std::thread::hardware_concurrency();
  cdbdirect_apply(handle, num_threads, evaluate_entry);

  cdbdirect_finalize(handle);
  return 0;
}

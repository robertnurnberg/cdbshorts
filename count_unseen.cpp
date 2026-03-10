#include "cdbdirect.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "external/chess.hpp"
using namespace chess;

constexpr int MAX_PLIES = 1000;

// cdb does not store tb7, or (stale)mates
std::vector<std::pair<std::string, int>>
cdbdirect_wrapper(std::uintptr_t handle, const chess::Board &board) {
  if (board.occ().count() <= 7 || !chess::movegen::anylegalmoves(board))
    return {{"a0a0", -3}};
  return cdbdirect_get(handle, board.getFen(false));
}

int main(int argc, char *argv[]) {
  std::uintptr_t handle = cdbdirect_initialize(CHESSDB_PATH);

  std::uint64_t db_size = cdbdirect_size(handle);
  std::cout << "DB count: " << db_size << std::endl;

  size_t max_entries = 1'000'000'000UL;
  int max_plies = 10;
  bool write_fens = true;
  if (argc > 1) { // either pass max_entries as integer
    size_t cli = std::stoull(argv[1]);
    if (cli > 1 || std::string(argv[1]) == "1") {
      max_entries = cli;
      if (max_entries == 1)
        std::cout << "Use '" << argv[0] << " 1.0' to analyse the whole DB."
                  << std::endl;
    } else { // or pass a fraction of total DB, like 0.5 or 1.0
      double fraction = std::stod(argv[1]);
      max_entries = std::size_t(fraction * db_size);
    }
    if (argc > 2)
      max_plies = std::stoi(argv[2]);
    if (argc > 3)
      write_fens = false;
  }
  max_entries = std::clamp(max_entries, 0UL, db_size);
  std::cout << "Analyse the first " << max_entries << " DB entries ..."
            << std::endl;
  max_plies = std::clamp(max_plies, 0, MAX_PLIES);
  std::cout << "Analyse positions up to min_ply depth " << max_plies
            << std::endl;

  // setup of a function that will be called for each entry in the db,
  // multithreaded
  std::atomic<size_t> count_total(0);
  std::array<std::atomic<size_t>, MAX_PLIES + 1> count_ply_unseen = {};
  std::array<std::atomic<size_t>, MAX_PLIES + 1> count_ply_total = {};
  std::mutex io_mutex;
  std::ofstream file_fens;
  if (write_fens)
    file_fens.open("unseen.epd");
  auto start = std::chrono::steady_clock::now();

  auto evaluate_entry =
      [&](const std::string &fen,
          const std::vector<std::pair<std::string, int>> &scored) {
        // count entries
        size_t peek = count_total.fetch_add(1, std::memory_order_relaxed);

        // status update
        if (peek % 1000000 == 0 && peek > 0) {
          auto end = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
              end - start);
          std::cout << "Counted                  " << peek << " of "
                    << max_entries << " entries so far..."
                    << "\n";
          for (int i = 0; i <= max_plies; i++) {
            const auto c = count_ply_unseen[i].load();
            if (c)
              std::cout << "  Ply " << std::setw(2) << i
                        << " fens w/ unseen: " << c << " ("
                        << c * 100.0 / count_ply_total[i].load() << "%)\n";
          }
          std::cout << "  Time (s):              " << elapsed.count() / 1000.0
                    << "\n";
          std::cout << "  nps:                   "
                    << peek * 1000 / elapsed.count() << "\n";
          std::cout << "  ETA (s):               "
                    << (max_entries - peek) * elapsed.count() / (peek * 1000)
                    << "\n";
          std::cout << std::endl;
        }

        int ply = scored.back().second;
        if (ply < 0 || ply > max_plies)
          return peek < max_entries;

        count_ply_total[ply].fetch_add(1, std::memory_order_relaxed);

        Board board(fen);
        Movelist moves;
        movegen::legalmoves(moves, board);
        for (const auto &move : moves) {
          auto uci_move = uci::moveToUci(move);
          auto it = std::find_if(
              scored.begin(), scored.end(),
              [&uci_move](const auto &p) { return p.first == uci_move; });
          if (it != scored.end())
            continue;
          board.makeMove<true>(move);
          auto r = cdbdirect_wrapper(handle, board);
          board.unmakeMove(move);
          if (r.size() > 1) {
            count_ply_unseen[ply].fetch_add(1, std::memory_order_relaxed);
            if (write_fens) {
              const std::lock_guard<std::mutex> lock(io_mutex);
              file_fens << fen << " ; min_ply: " << ply << "\n";
            }
            break;
          }
        }

        return peek < max_entries; // continue iteration as long as true
      };

  // evaluate all entries in the db, using multiple threads
  const size_t num_threads = std::thread::hardware_concurrency();
  cdbdirect_apply(handle, num_threads, evaluate_entry);

  std::cout << "Counted " << max_entries << " entries.\n";
  for (int i = 0; i <= max_plies; i++) {
    const auto c = count_ply_unseen[i].load();
    if (c)
      std::cout << "  Ply " << std::setw(2) << i << " fens w/ unseen: " << c
                << " (" << c * 100.0 / count_ply_total[i].load() << "%)\n";
  }

  if (write_fens)
    file_fens.close();

  cdbdirect_finalize(handle);
  return 0;
}

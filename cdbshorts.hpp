#include "cdbdirect.h"
#include <string>
#include <utility>
#include <vector>

#include "external/chess.hpp"

// cdb does not store tb7, or (stale)mates
std::vector<std::pair<std::string, int>>
cdbdirect_wrapper(std::uintptr_t handle, const chess::Board &board) {
  if (board.occ().count() <= 7 || !chess::movegen::anylegalmoves(board))
    return {{"a0a0", -3}};
  return cdbdirect_get(handle, board.getXfen(false));
}

// for chess960 chess::uci::uciToMove only accepts KxR castling notation
chess::Move cdbuci_to_move(const chess::Board &board, std::string_view uci) {
  if (board.chess960() &&
      (uci == "e1g1" || uci == "e1c1" || uci == "e8g8" || uci == "e8c8")) {
    auto source = chess::Square(uci.substr(0, 2));
    auto target = chess::Square(uci.substr(2, 2));
    auto pt = board.at(source).type();

    if (pt == chess::PieceType::KING &&
        board.at(target).type() == chess::PieceType::NONE) {
      target = chess::Square(target > source ? chess::File::FILE_H
                                             : chess::File::FILE_A,
                             source.rank());
      return chess::Move::make<chess::Move::CASTLING>(source, target);
    }
  }
  return chess::uci::uciToMove(board, uci);
}

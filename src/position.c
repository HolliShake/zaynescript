#include "./position.h"

Position PositionFromLineAndColm(int line, int colm) {
    Position position = {
        .LineStart = line,
        .LineEnded = line,
        .ColmStart = colm,
        .ColmEnded = colm
    };
    return position;
}


Position MergePositions(Position a, Position b) {
    Position position;
    position.LineStart = a.LineStart < b.LineStart ? a.LineStart : b.LineStart;
    position.LineEnded = a.LineEnded > b.LineEnded ? a.LineEnded : b.LineEnded;
    position.ColmStart = a.ColmStart < b.ColmStart ? a.ColmStart : b.ColmStart;
    position.ColmEnded = a.ColmEnded > b.ColmEnded ? a.ColmEnded : b.ColmEnded;
    return position;
}
#include "./position.h"


Position PositionFromLineAndColm(int line, int colm) {
    Position position = {
        .lineStart = line,
        .lineEnded = line,
        .colmStart = colm,
        .colmEnded = colm
    };
    return position;
}


Position MergePositions(Position a, Position b) {
    Position position;
    position.lineStart = a.lineStart < b.lineStart ? a.lineStart : b.lineStart;
    position.lineEnded = a.lineEnded > b.lineEnded ? a.lineEnded : b.lineEnded;
    position.colmStart = a.colmStart < b.colmStart ? a.colmStart : b.colmStart;
    position.colmEnded = a.colmEnded > b.colmEnded ? a.colmEnded : b.colmEnded;
    return position;
}
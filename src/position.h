#include "global.h"

#ifndef POSITION_H
#define POSITION_H


/**
 * Creates a position structure from line and column numbers
 * 
 * @param line The line number in the source file
 * @param colm The column number in the source file
 * @return Position structure with both start and end set to the given line and column
 */
Position PositionFromLineAndColm(int line, int colm);

/**
 * Merges two position structures into a single position
 * 
 * @param a First position structure
 * @param b Second position structure
 * @return Merged position structure spanning from the earliest start to the latest end
 */
Position MergePositions(Position a, Position b);

#endif
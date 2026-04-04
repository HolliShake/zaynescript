/**
 * @file position.h
 * @brief Source code position tracking for error reporting and
 * debugging.
 *
 * Provides utilities for creating and combining Position
 * structures that associate tokens, AST nodes, and errors with
 * their original locations in the source file. All line and
 * column numbers are 1-based.
 */

#include "global.h"

#ifndef POSITION_H
#	define POSITION_H

/**
 * @brief Creates a Position representing a single point in the
 * source code.
 *
 * Constructs a Position where both start and end are set to the
 * same line and column, representing a zero-width location at
 * that point.
 *
 * @param line Line number in the source file (1-based).
 * @param colm Column number in the source file (1-based).
 * @return Position structure with LineStart, LineEnded,
 * ColmStart, and ColmEnded all set to the given line and column.
 */
Position PositionFromLineAndColm(int line, int colm);

/**
 * @brief Merges two positions into a single span covering both.
 *
 * Combines two Position structures to produce a new position
 * whose start is the earlier of the two starts and whose end is
 * the later of the two ends. Used when constructing compound AST
 * nodes that span multiple tokens.
 *
 * @param a First Position (typically from the leading token or
 * sub-expression).
 * @param b Second Position (typically from the trailing token or
 * sub-expression).
 * @return Merged Position spanning from the earliest start to
 * the latest end.
 */
Position MergePositions(Position a, Position b);

#endif
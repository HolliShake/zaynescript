#include "global.h"

#ifndef POSITION_H
#define POSITION_H

/*
 * Source code position tracking for error reporting and debugging
 * 
 * This module provides functionality for tracking positions in source code files.
 * Positions are used throughout the compiler to associate tokens, AST nodes, and
 * errors with their original locations in the source code, enabling accurate
 * error messages and debugging information.
 */

/*
 * PositionFromLineAndColm - Creates a position structure from line and column numbers
 * 
 * Constructs a Position structure representing a single point in the source code.
 * Both the start and end of the position are set to the same line and column,
 * representing a zero-width position at the specified location.
 * 
 * line: The line number in the source file (1-indexed)
 * colm: The column number in the source file (1-indexed)
 * 
 * Returns: Position structure with both start and end set to the given line and column
 */
Position PositionFromLineAndColm(int line, int colm);

/*
 * MergePositions - Merges two position structures into a single position
 * 
 * Combines two Position structures to create a new position that spans from
 * the earliest start point to the latest end point. This is useful for
 * representing the position of a compound expression or statement that
 * encompasses multiple sub-elements.
 * 
 * The resulting position will have:
 * - Start line and column from the earlier position
 * - End line and column from the later position
 * 
 * a: First position structure
 * b: Second position structure
 * 
 * Returns: Merged position structure spanning from the earliest start to the latest end
 */
Position MergePositions(Position a, Position b);

#endif
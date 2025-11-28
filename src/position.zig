const std = @import("std");

/// Position tracks the location of a token in the source code
/// Used for error reporting and debugging
pub const Position = struct {
    line_start: u32,
    line_ended: u32,
    colm_start: u32,
    colm_ended: u32,

    /// Creates a new Position
    pub fn init(line_start: u32, line_ended: u32, colm_start: u32, colm_ended: u32) Position {
        return Position{
            .line_start = line_start,
            .line_ended = line_ended,
            .colm_start = colm_start,
            .colm_ended = colm_ended,
        };
    }

    /// Merges two position ranges for compound tokens
    /// Returns a new Position spanning from this.Start to other.End
    pub fn merge(self: Position, other: Position) Position {
        return Position{
            .line_start = self.line_start,
            .line_ended = other.line_ended,
            .colm_start = self.colm_start,
            .colm_ended = other.colm_ended,
        };
    }

    /// Formats the position for display
    pub fn format(
        self: Position,
        comptime fmt: []const u8,
        _: anytype,
        writer: anytype,
    ) !void {
        _ = fmt;
        try writer.print("{}:{}-{}:{}", .{
            self.line_start,
            self.colm_start,
            self.line_ended,
            self.colm_ended,
        });
    }
};


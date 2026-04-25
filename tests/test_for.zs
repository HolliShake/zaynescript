import { println } from "core:io";

println("=== test_for stress start ===");

// TAKENOTE:
// In this language, switch statements do not have break/continue features.
// break/continue below are loop controls, used outside switch behavior.
fn runStress(limit) {
    local sum = 0;
    local continues = 0;
    local processed = 0;
    local tail = 0;
    local caught = 0;
    local breaks = 0;

    for (i := 0; i < limit; i++) {
        try {
            switch (i % 4) {
                case 0: {
                    continues += 1;
                    continue;
                }
                case 1: {
                    processed += 1;
                    sum += i;
                }
                case 2: {
                    processed += 1;
                    if (i == 10) {
                        raise "forced-error";
                    }
                    sum += i * 2;
                }
                default: {
                    processed += 1;
                    if (i >= 15) {
                        breaks += 1;
                        break;
                    }
                    sum += 3;
                }
            }

            // Reached only when not continued, not raised, and not broken.
            tail += 1;
        } catch (e) {
            caught += 1;
            assert e == "forced-error", "unexpected error payload";
            continue;
        }
    }

    return {
        sum,
        continues,
        processed,
        tail,
        caught,
        breaks
    };
}

fn firstMatchWithReturn(limit) {
    for (i := 0; i < limit; i++) {
        switch (i) {
            case 0, 1, 2: {
                continue;
            }
            default: {
                if (i == 7) {
                    return i;
                }
            }
        }
    }
    return -1;
}

fn runNestedStress() {
    local score = 0;
    local innerContinues = 0;
    local innerBreaks = 0;
    local outerBreaks = 0;
    local caught = 0;
    local tail = 0;

    for (i := 0; i < 6; i++) {
        try {
            for (j := 0; j < 7; j++) {
                try {
                    switch ((i + j) % 5) {
                        case 0: {
                            innerContinues += 1;
                            continue;
                        }
                        case 1: {
                            score += i + j;
                        }
                        case 2: {
                            if (i == 2 && j == 0) {
                                raise "nested-boom";
                            }
                            score += 2;
                        }
                        case 3: {
                            innerBreaks += 1;
                            break;
                        }
                        default: {
                            score += 1;
                        }
                    }

                    tail += 1;
                } catch (e) {
                    caught += 1;
                    assert e == "nested-boom", "nested error payload mismatch";
                    continue;
                }
            }

            if (i == 4) {
                outerBreaks += 1;
                break;
            }
        } catch (e) {
            assert false, "outer try in nested stress should not catch";
        }
    }

    return {
        score,
        innerContinues,
        innerBreaks,
        outerBreaks,
        caught,
        tail
    };
}

fn nestedReturnTarget() {
    for (i := 0; i < 6; i++) {
        for (j := 0; j < 6; j++) {
            switch (i + j) {
                case 0, 1: {
                    continue;
                }
                default: {
                    if (i == 3 && j == 2) {
                        return i * 10 + j;
                    }
                }
            }
        }
    }
    return -1;
}

println(20);
const result = runStress(20);
assert result.sum == 81, "sum mismatch";
assert result.continues == 4, "continue count mismatch";
assert result.processed == 12, "processed count mismatch";
assert result.tail == 10, "tail count mismatch";
assert result.caught == 1, "catch count mismatch";
assert result.breaks == 1, "break count mismatch";

const found = firstMatchWithReturn(20);
assert found == 7, "return-from-loop mismatch";

const nested = runNestedStress();
assert nested.score == 17, "nested score mismatch";
assert nested.innerContinues == 2, "nested continue count mismatch";
assert nested.innerBreaks == 5, "nested inner break count mismatch";
assert nested.outerBreaks == 1, "nested outer break count mismatch";
assert nested.caught == 1, "nested catch count mismatch";
assert nested.tail == 8, "nested tail count mismatch";

const nestedFound = nestedReturnTarget();
assert nestedFound == 32, "nested return target mismatch";

println("PASS test_for stress");
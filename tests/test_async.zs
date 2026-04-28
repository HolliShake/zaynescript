import { println } from "core:io";

fn resolve(v) async {
    return v;
}

fn reject(v) async {
    raise v;
}

fn addAsync(a, b) async {
    const x = await resolve(a);
    const y = await resolve(b);
    return x + y;
}

// -------------------------------------------------------------
// 1) await order: in-loop pre/await/post should be deterministic
// -------------------------------------------------------------
var orderLog = "";
fn mark(tag) {
    orderLog = orderLog + tag + "|";
    return tag;
}

fn stressAwaitOrder() async {
    orderLog = "";
    mark("start");
    for (i := 0; i < 100; i++) {
        mark("pre");
        const v = await resolve(i);
        assert v == i, "await order: awaited value matches loop index";
        mark("post");
    }
    mark("done");
    return true;
}

var orderDone = false;
stressAwaitOrder().then(fn(ok) {
    orderDone = ok;
});
assert orderDone, "await order: stress function completes";
assert orderLog != "", "await order: trace string is populated";

// -------------------------------------------------------------
// 2) await rejection: try/catch must catch every rejection
// -------------------------------------------------------------
var awaitCaught = 0;
var awaitSucceeded = 0;
var awaitLastError = "";

fn stressAwaitRejectCatch() async {
    awaitCaught = 0;
    awaitSucceeded = 0;
    awaitLastError = "";

    for (i := 0; i < 100; i++) {
        try {
            await reject("await-err");
            awaitSucceeded++;
        } catch (e) {
            awaitCaught++;
            awaitLastError = e;
        }
    }
    return true;
}

var awaitRejectDone = false;
stressAwaitRejectCatch().then(fn(ok) {
    awaitRejectDone = ok;
});
assert awaitRejectDone, "await reject: stress function completes";
assert awaitCaught == 100, "await reject: every rejection is caught";
assert awaitSucceeded == 0, "await reject: success path never runs";
assert awaitLastError != "", "await reject: catch receives rejection reason";

// -------------------------------------------------------------
// 3) .then chain ordering and value forwarding
// -------------------------------------------------------------
var thenOrderCheck = 0;
var thenFinal = 0;

for (j := 0; j < 80; j++) {
    resolve(j)
        .then(fn(v) {
            thenOrderCheck += 1;
            return v + 1;
        })
        .then(fn(v) {
            thenOrderCheck += 10;
            return v * 2;
        })
        .then(fn(v) {
            thenOrderCheck += 100;
            thenFinal += v;
        });
}

assert thenOrderCheck == 8880, ".then: all chain stages run in full";
assert thenFinal == 6480, ".then: final accumulated value matches expected math";

// -------------------------------------------------------------
// 4) JS-like error propagation:
//    throw in .then -> .error gets it -> recovery value reaches next .then
// -------------------------------------------------------------
var propagateThenCount = 0;
var propagateErrorCount = 0;
var propagateRecoveredCount = 0;
var propagateLastError = "";

for (k := 0; k < 80; k++) {
    resolve(k)
        .then(fn(v) {
            propagateThenCount++;
            raise "then-err";
        })
        .error(fn(e) {
            propagateErrorCount++;
            propagateLastError = e;
            return "recovered";
        })
        .then(fn(v) {
            if (v == "recovered") {
                propagateRecoveredCount++;
            }
        });
}

assert propagateThenCount == 80, "propagation: throwing .then callbacks all execute";
assert propagateErrorCount == 80, "propagation: .error catches every thrown .then error";
assert propagateRecoveredCount == 80, "propagation: recovery value flows into next .then";
assert propagateLastError != "", "propagation: error value preserved";

// -------------------------------------------------------------
// 5) Rejected promise should skip .then and go to .error
// -------------------------------------------------------------
var rejectThenCount = 0;
var rejectErrorCount = 0;
var rejectRecoveredCount = 0;

for (n := 0; n < 80; n++) {
    reject("reject")
        .then(fn(_) {
            println(">>@@");
            rejectThenCount++;
            return rejectThenCount;
        })
        .error(fn(e) {
            rejectErrorCount++;
            assert e != "", "reject path: .error receives reject reason";
            return 7;
        })
        .then(fn(v) {
            if (v == 7) {
                rejectRecoveredCount++;
            }
        });
}

assert rejectThenCount == 0, "reject path: initial .then is skipped";
assert rejectErrorCount == 80, "reject path: all rejections are caught";
assert rejectRecoveredCount == 80, "reject path: .error recovery continues chain";

// -------------------------------------------------------------
// 6) Mixed await + then stress sanity
// -------------------------------------------------------------
fn mixedStress() async {
    local total = 0;
    for (m := 0; m < 80; m++) {
        const v = await addAsync(m, 1);
        total += v;
    }
    return total;
}

var mixedTotal = 0;
mixedStress()
    .then(fn(v) {
        mixedTotal = v;
    })
    .error(fn(e) {
        assert false, "mixed stress: should not error";
    });

assert mixedTotal == 3240, "mixed stress: await + then total is stable";

println("test_async: stress checks passed");

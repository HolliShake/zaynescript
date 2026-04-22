import { println } from "core:io";
import { compile, search, match, fullmatch, findall, I } from "core:regex";

println("Testing core:regex (python-like API)...");

// module-level helpers
const s1 = search("cat", "xxcatyy");
assert s1 != null, "regex: search finds first match";
assert s1[0] == "cat", "regex: search match text";

assert match("cat", "xxcatyy") == null, "regex: match anchors at start";
const m1 = match("cat", "catyy");
assert m1 != null && m1[0] == "cat", "regex: match at start succeeds";

const fm1 = fullmatch("cat", "cat");
assert fm1 != null && fm1[0] == "cat", "regex: fullmatch exact";
assert fullmatch("cat", "cat!") == null, "regex: fullmatch rejects extra chars";

// compiled pattern usage + flag
const rx = compile("(ab)+", I);
const s2 = rx.search("..ABab..");
assert s2 != null && s2[0] == "ABab", "regex: compile + IGNORECASE";
assert rx.match("zzAB") == null, "regex: compiled match anchored";
const fm2 = rx.fullmatch("ABab");
assert fm2 != null && fm2[0] == "ABab", "regex: compiled fullmatch";

// findall return shapes (python-like)
const f1 = findall("a.", "abac");
assert f1[0] == "ab" && f1[1] == "ac", "regex: findall no groups";

const f2 = findall("a(.)", "abac");
assert f2[0] == "b" && f2[1] == "c", "regex: findall single group";

const f3 = findall("(a)(.)", "abac");
assert f3[0][0] == "a" && f3[0][1] == "b", "regex: findall multi-group row 1";
assert f3[1][0] == "a" && f3[1][1] == "c", "regex: findall multi-group row 2";

println("regex tests passed");

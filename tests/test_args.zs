import { args } from "core:os";
import { println } from "core:io";
import { ArgParser, parseWithDefs, createArgParser } from "lib:argparser";

/*
   Example:
   ./dist/zscript.exe --run ./tests/test_args.zs adasd -f asdad --file=2323
   argv from args(): ["adasd", "-f", "asdad", "--file=2323"]
*/

println(ArgParser);

const argv = args();

const p = new ArgParser();
p.option({ long: "file", short: "f", takesValue: true, help: "path or value" });
p.option({ long: "verbose", short: "v", takesValue: false, help: "verbose" });
p.option({ long: "count", short: "c", takesValue: true, defaultVal: "1", help: "repeat count" });

const r = p.parse(argv);

println("argv: ", argv);
println("ok: ", r.ok);
println("positional: ", r.positional);
println("flags: ", r.flags);
println("unknown: ", r.unknown);
if (!r.ok) {
    println("errors: ", r.errors);
}

println("");
println(p.helpText("mytool"));

/* Quick regression via parseWithDefs */
const r2 = parseWithDefs([
    { long: "out", short: "o", takesValue: true },
    { long: "quiet", short: "q" }
], ["-qo", "x.y", "rest"]);
println("parseWithDefs positional:", r2.positional, "flags:", r2.flags);

// core:io File — read/write/seek, text modes, Blob raw I/O (assert-based)

import { File } from "core:io";
import { Blob } from "core:blob";

const path = "/tmp/zs_test_fileio.txt";

// --- write mode: writable, not readable ---
var w = new File(path, "w");
assert w.isClosed() == false, "fileio: open file not closed";
assert w.writable() == true, "fileio: w mode writable";
assert w.readable() == false, "fileio: w mode not readable";
w.writelines(["line1\n", "line2\n"]);
w.flush();
w.close();
assert w.isClosed() == true, "fileio: closed after close";

// --- read all ---
var r = new File(path, "r");
assert r.readable() == true, "fileio: r readable";
assert r.writable() == false, "fileio: r not writable";
assert r.read() == "line1\nline2\n", "fileio: read entire file";
assert r.readline() == "", "fileio: readline at EOF empty";
r.close();

// --- readlines ---
var r2 = new File(path, "r");
const lines = r2.readlines();
assert lines.length() == 2, "fileio: readlines two lines";
assert lines[0] == "line1\n", "fileio: readlines[0]";
assert lines[1] == "line2\n", "fileio: readlines[1]";
r2.close();

// --- r+ seek / tell / partial read ---
var rw = new File(path, "r+");
assert rw.readable() == true && rw.writable() == true, "fileio: r+ read+write";
assert rw.tell() == 0, "fileio: tell at start";
rw.seek(6, 0);
assert rw.tell() == 6, "fileio: tell after SEEK_SET";
assert rw.read(1) == "2", "fileio: read one char";
rw.seek(0, 2);
const endPos = rw.tell();
assert endPos > 10, "fileio: tell at SEEK_END";
rw.close();

// --- read(0) empty string ---
var r3 = new File(path, "r");
assert r3.read(0) == "", "fileio: read(0) empty";
r3.close();

// --- write(Blob): raw bytes on disk; read as text ---
const binPath = "/tmp/zs_test_fileio_blob.bin";
const payload = new Blob([65, 66, 10], { type: "application/octet-stream" });
var bw = new File(binPath, "wb");
bw.write(payload);
bw.close();
var br = new File(binPath, "r");
assert br.read() == "AB\n", "fileio: write(Blob) raw bytes";
br.close();

// --- writelines: Blob elements write payload, not debug text ---
const mixPath = "/tmp/zs_test_fileio_mix.txt";
var wm = new File(mixPath, "w");
wm.writelines([new Blob([">"]), new Blob(["<"]), "!\n"]);
wm.close();
var rm = new File(mixPath, "r");
assert rm.read() == "><!\n", "fileio: writelines with Blob parts";
rm.close();

// --- writelines in binary mode: concatenate blob payloads ---
const wlBin = "/tmp/zs_test_fileio_wlbin.bin";
var wb = new File(wlBin, "wb");
wb.writelines([new Blob([77, 78]), new Blob([79])]);
wb.close();
var rb = new File(wlBin, "r");
assert rb.read() == "MNO", "fileio: writelines binary mode blob concat";
rb.close();

import { Array }  from "core:array";
import { Object } from "core:object";
import { Blob } from "core:blob";
import { MimeTypes } from "core:blob";
import { println }  from  "core:io";


println(new Array(1,2,3),new Blob(["hello", new Blob(["hello"])], { type: MimeTypes.TEXT_PLAIN }), new Object());
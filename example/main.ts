import * as fs from "fs";
import "p";

const start = performance.now();
const content = fs.readFileSync("../test/large-file.json", "utf8");
JSON.parse(content);

console.log("done.", (performance.now() - start) / 1000);

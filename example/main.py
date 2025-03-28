import json
import time

start = time.time()
content = json.load(open("../test/large-file.json"))
print("done.", time.time() - start)

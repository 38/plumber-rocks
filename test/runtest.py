import json
import sys
import subprocess
import signal
import os
import tempfile
import shutil
def parse_out(outfile, verify, error):
    lines = [line.strip() for line in outfile]
    buf = None
    label = ""
    result = True
    for line in lines:
        if line[:8] == ".OUTPUT ":
            buf = []
            label = line[8:].strip()
        elif line == ".END":
            try:
                if not verify(label, json.loads("".join(buf))):
                    result = False
                    print "Test case {label} failed".format(label = label)
                else:
                    print "Test case {label} succeeded".format(label = label)
            except Exception as ex:
                error(label, ex)
                result = False
            buf = None
        elif buf != None:
            buf.append(line)
    return result


dbpath = tempfile.mkdtemp()
def test_case(script, servlet_dir, case_name):
    def verify(l,o):
        for name in o:
            if name not in expected[l] or expected[l][name] != o[name]:
                return False
        for name in expected[l]:
            if name not in o:
                return False
        return True
    case_name = case_name.rsplit(".", 1)[0]
    args = ["valgrind", 
            "--errors-for-leak-kinds=definite", 
            "--leak-check=full", 
            "--error-exitcode=1", 
            "pscript", 
            script, 
            servlet_dir , 
            "{case_name}.in".format(case_name = case_name), 
            "/dev/stdout",
            dbpath]
    sys.stderr.write(" ".join(args) + "\n")
    proc = subprocess.Popen(args, stdout = subprocess.PIPE)
    expected = dict([(lambda (x,y): (x, json.loads(y)))(line.strip().split("\t")) for line in open("{case_name}.out".format(case_name = case_name))])
    exit_code = proc.wait()
    return parse_out(proc.stdout, verify, lambda l, e: None) and exit_code == 0

result = True

for i in sys.argv[2:]:
    result = result and test_case(sys.argv[1], "bin/rocksdb", i)

shutil.rmtree(dbpath)

if not result:
    sys.exit(1)

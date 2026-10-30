# Usage: add-symbol-file-all <filename> [<offset>]
#        remove-symbol-file-all <filename> [<offset>]
#
# Credit: https://stackoverflow.com/a/33087762/9352057
#         CC BY-SA 4.0

python
import subprocess
import re
import os
import shutil

def relocatesections(filename, addr):
    p = subprocess.Popen(["readelf", "-S", filename], stdout = subprocess.PIPE)

    sections = []
    textaddr = '0'
    for line in p.stdout.readlines():
        line = line.decode("utf-8").strip()
        if not line.startswith('[') or line.startswith('[Nr]'):
            continue

        line = re.sub(r' +', ' ', line)
        line = re.sub(r'\[ *(\d+)\]', '\g<1>', line)
        fieldsvalue = line.split(' ')
        fieldsname = ['number', 'name', 'type', 'addr', 'offset', 'size', 'entsize', 'flags', 'link', 'info', 'addralign']
        sec = dict(zip(fieldsname, fieldsvalue))

        if sec['number'] == '0':
            continue

        sections.append(sec)

        if sec['name'] == '.text':
            textaddr = sec['addr']

    return (textaddr, sections)

def load_rust_pretty_printers():
    rustc = os.environ.get("RUSTC") or shutil.which("rustc")

    if not rustc:
        print("Fennix GDB: rustc not found in PATH")
        return

    try:
        sysroot = subprocess.check_output(
            [rustc, "--print", "sysroot"],
            text=True
        ).strip()

        printer = os.path.join(
            sysroot,
            "lib",
            "rustlib",
            "etc",
            "gdb_load_rust_pretty_printers.py"
        )

        if not os.path.isfile(printer):
            print("Fennix GDB: Rust pretty-printers not found:", printer)
            return

        gdb.execute("source " + printer)
        print("Fennix GDB: Rust pretty-printers loaded")

    except gdb.error as error:
        print("Fennix GDB: Failed to load Rust pretty-printers:", error)

class AddSymbolFileAll(gdb.Command):
    """The right version for add-symbol-file"""

    def __init__(self):
        super(AddSymbolFileAll, self).__init__("add-symbol-file-all", gdb.COMMAND_USER)
        self.dont_repeat()

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        filename = argv[0]

        if len(argv) > 1:
            offset = int(str(gdb.parse_and_eval(argv[1])), 0)
        else:
            offset = 0

        (textaddr, sections) = relocatesections(filename, offset)

        cmd = "add-symbol-file %s 0x%08x" % (filename, int(textaddr, 16) + offset)

        for s in sections:
            addr = int(s['addr'], 16)
            if s['name'] == '.text' or addr == 0:
                continue

            cmd += " -s %s 0x%08x" % (s['name'], addr + offset)

        gdb.execute(cmd)

class RemoveSymbolFileAll(gdb.Command):
    """The right version for remove-symbol-file"""

    def __init__(self):
        super(RemoveSymbolFileAll, self).__init__("remove-symbol-file-all", gdb.COMMAND_USER)
        self.dont_repeat()

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        filename = argv[0]

        if len(argv) > 1:
            offset = int(str(gdb.parse_and_eval(argv[1])), 0)
        else:
            offset = 0

        (textaddr, _) = relocatesections(filename, offset)

        cmd = "remove-symbol-file -a 0x%08x" % (int(textaddr, 16) + offset)
        gdb.execute(cmd)


AddSymbolFileAll()
RemoveSymbolFileAll()
load_rust_pretty_printers()
end

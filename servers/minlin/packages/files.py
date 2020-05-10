import sys; sys.path.append('..')
from build import Package
import os

FILES = {}
FILES["/etc/banner"] = r"""
 ____
/\  _`\
\ \ \L\ \     __    ____     __     __
 \ \ ,  /   /'__`\ /',__\  /'__`\ /'__`\
  \ \ \\ \ /\  __//\__, `\/\  __//\ \L\.\_
   \ \_\ \_\ \____\/\____/\ \____\ \__/.\_\
    \/_/\/ /\/____/\/___/  \/____/\/__/\/_/
""".lstrip()

class Files(Package):
    def __init__(self):
        super().__init__()
        self.name = "files"
        self.version = ""
        self.url = None
        self.host_deps = []
        self.files = { path: path.lstrip("/") for path in FILES.keys() }

    def build(self):
        for path, content in FILES.items():
            self.add_file(path.lstrip("/"), content)

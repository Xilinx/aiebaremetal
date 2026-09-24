'''
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
'''
import http.server
import socketserver
import os
from functools import partial

class SPASimpleHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, directory=None, **kwargs):
        super().__init__(*args, directory=directory, **kwargs)

    def send_head(self):
        # Try normal resolution first
        path = self.translate_path(self.path)
        if os.path.isdir(path):
            for index in ("index.html", "index.htm"):
                index_path = os.path.join(path, index)
                if os.path.exists(index_path):
                    self.path = os.path.join(self.path.rstrip("/"), index)
                    break
        # Fall back to index.html for non-existent paths (SPA)
        try:
            return super().send_head()
        except Exception:
            self.path = "/index.html"
            return super().send_head()

def serve_spa(root_dir=".", host="127.0.0.1", port=8000):
    Handler = partial(SPASimpleHandler, directory=root_dir)
    with socketserver.TCPServer((host, port), Handler) as httpd:
        print(f"Serving SPA from {os.path.abspath(root_dir)} at http://{host}:{port}")
        httpd.serve_forever()

if __name__ == "__main__":
    serve_spa(root_dir="tmp/html/", host="0.0.0.0", port=8000)

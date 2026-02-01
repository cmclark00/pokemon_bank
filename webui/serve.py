#!/usr/bin/env python3
"""
Simple HTTP server for Pokemon Bank WebUI testing.
Run this script and open http://localhost:8000 in Chrome/Edge.
WebSerial requires localhost or HTTPS.
"""

import http.server
import socketserver
import os
import sys

PORT = 8000

# Change to the webui directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for local development
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        super().end_headers()

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print(f"Pokemon Bank WebUI Server")
    print(f"Serving at http://localhost:{PORT}")
    print(f"Press Ctrl+C to stop")
    print()
    print("NOTE: Use Chrome or Edge for WebSerial support")
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        sys.exit(0)

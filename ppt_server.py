import http.server
import socketserver
import subprocess
import sys
import socket

PORT = 8765

class PPTRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        client_ip = self.client_address[0]
        print(f"\n[{client_ip}] Request: {self.path}")
        
        action = None
        if 'action=next' in self.path:
            action = "NEXT"
            print(f"  -> PPT Next Slide")
            self.send_keys(['ctrl', 'right'])
        elif 'action=prev' in self.path:
            action = "PREV"
            print(f"  -> PPT Previous Slide")
            self.send_keys(['ctrl', 'left'])
        elif 'action=volume_up' in self.path:
            action = "VOLUME_UP"
            print(f"  -> Volume Up")
            self.send_keys(['volumeup'])
        elif 'action=volume_down' in self.path:
            action = "VOLUME_DOWN"
            print(f"  -> Volume Down")
            self.send_keys(['volumedown'])
        else:
            print(f"  -> Unknown action: {self.path}")
        
        response = b"OK"
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.send_header('Content-Length', str(len(response)))
        self.end_headers()
        self.wfile.write(response)
        
        if action:
            print(f"  -> OK ({action})")
    
    def send_keys(self, keys):
        try:
            import pyautogui
            if len(keys) > 1:
                pyautogui.hotkey(*keys)
            else:
                pyautogui.press(keys[0])
        except ImportError:
            print("  Error: pyautogui not installed")
        except Exception as e:
            print(f"  Error sending keys: {e}")
    
    def log_message(self, format, *args):
        pass

def get_local_ips():
    ips = []
    try:
        hostname = socket.gethostname()
        addrs = socket.getaddrinfo(hostname, None)
        for addr in addrs:
            ip = addr[4][0]
            if '.' in ip and not ip.startswith('127.'):
                ips.append(ip)
    except:
        pass
    return ips

if __name__ == "__main__":
    try:
        import pyautogui
        print("pyautogui is ready")
    except ImportError:
        print("Installing pyautogui...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "pyautogui"])
        import pyautogui
    
    ips = get_local_ips()
    
    print("=" * 50)
    print(f"PPT Controller Server starting on port {PORT}...")
    print(f"Server is listening on: 0.0.0.0:{PORT}")
    print()
    print("Local IP addresses:")
    for ip in ips:
        print(f"  - {ip}")
    print()
    print("Set ESP32 PPT Server IP to one of the above")
    print("Commands supported: next, prev, volume_up, volume_down")
    print("Press Ctrl+C to stop")
    print("=" * 50)
    print()
    
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("0.0.0.0", PORT), PPTRequestHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nServer stopped")
            httpd.shutdown()

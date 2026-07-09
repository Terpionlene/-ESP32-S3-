import socket
import urllib.parse
import ctypes

VK_PRIOR = 0x21
VK_NEXT = 0x22
VK_VOLUME_UP = 0xAF
VK_VOLUME_DOWN = 0xAE

KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_EXTENDEDKEY = 0x0001

def send_key(key_code):
    ctypes.windll.user32.keybd_event(key_code, 0, KEYEVENTF_EXTENDEDKEY, 0)
    ctypes.windll.user32.keybd_event(key_code, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0)

def handle_client(client_socket):
    try:
        request = client_socket.recv(1024).decode('utf-8')
        if not request:
            return
        
        lines = request.split('\r\n')
        if len(lines) == 0:
            return
        
        first_line = lines[0]
        parts = first_line.split(' ')
        if len(parts) < 2:
            return
        
        method = parts[0]
        url = parts[1]
        
        if method == 'GET' and '/control' in url:
            parsed = urllib.parse.urlparse(url)
            query = urllib.parse.parse_qs(parsed.query)
            
            if 'action' in query:
                action = query['action'][0]
                
                if action == 'next':
                    print("Received: Next Page")
                    send_key(VK_NEXT)
                elif action == 'prev':
                    print("Received: Previous Page")
                    send_key(VK_PRIOR)
                elif action == 'volume_up':
                    print("Received: Volume Up")
                    send_key(VK_VOLUME_UP)
                elif action == 'volume_down':
                    print("Received: Volume Down")
                    send_key(VK_VOLUME_DOWN)
                else:
                    print(f"Unknown action: {action}")
            
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"
        else:
            response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"
        
        client_socket.sendall(response.encode('utf-8'))
    except Exception as e:
        print(f"Error handling client: {e}")
    finally:
        client_socket.close()

def main():
    host = '0.0.0.0'
    port = 8765
    
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((host, port))
    server_socket.listen(5)
    
    print(f"PPT Controller Server started on {host}:{port}")
    print("Waiting for ESP32 connections...")
    
    while True:
        client_socket, addr = server_socket.accept()
        print(f"Connected from {addr}")
        handle_client(client_socket)

if __name__ == '__main__':
    main()
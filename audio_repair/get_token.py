import requests

API_KEY = "your_api_key"
SECRET_KEY = "your_secret_key"

url = f"https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id={API_KEY}&client_secret={SECRET_KEY}"

print(f"Getting token from: {url}")
print("=" * 60)

response = requests.get(url)
data = response.json()

if "access_token" in data:
    token = data["access_token"]
    expires_in = data.get("expires_in", 0)
    
    print(f"✅ Token obtained successfully!")
    print(f"Token: {token}")
    print(f"Expires in: {expires_in} seconds ({expires_in // 86400} days)")
    print("=" * 60)
    print(f"Copy this token to your ESP32 code:")
    print(f'const char* baidu_token = "{token}";')
else:
    print(f"❌ Failed to get token!")
    if "error" in data:
        print(f"Error: {data['error']}")
        if "error_description" in data:
            print(f"Description: {data['error_description']}")
    print(f"Response: {data}")
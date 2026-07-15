import requests

url = "https://tile.openstreetmap.org/15/26100/15390.png"

headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0 Safari/537.36",
    "Accept": "image/png,image/*,*/*;q=0.8",
}

r = requests.get(url, headers=headers)

print("Status:", r.status_code)
print("Content-Type:", r.headers.get("Content-Type"))
print(r.text[:300])

open("test.png", "wb").write(r.content)
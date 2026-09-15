import requests

url = "https://api.callmebot.com/whatsapp.php"

params = {
    "phone": "201099942942",
    "text": "Test From Python",
    "apikey": "5159529"
}

r = requests.get(url, params=params)

print("Status Code:", r.status_code)
print("Response:", r.text)
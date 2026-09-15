import serial
import csv
import os
import requests
import time
from datetime import datetime

# ================= SERIAL =================
arduino = serial.Serial('COM3', 9600)

# ================= WHATSAPP =================
API_KEY = "5159529"

contacts = {
    "0129": ["201099942942"],
    "0053": ["201014853488"],
    "0123": ["201553111407"],
    "0012": ["201156780165"]
}

# ================= CSV =================
file_exists = os.path.isfile('attendance.csv')

file = open('attendance.csv', 'a', newline='', encoding='utf-8')
writer = csv.writer(file)

if not file_exists:
    writer.writerow(["ID", "Name", "Date", "Time", "Status"])

print("Waiting for data...")

# ================= WHATSAPP =================
def send_whatsapp(phone, message):

    url = "https://api.callmebot.com/whatsapp.php"

    params = {
        "phone": phone,
        "text": message,
        "apikey": API_KEY
    }

    try:
        r = requests.get(url, params=params, timeout=10)

        print("Sent to:", phone, "| Status:", r.status_code)
        print("Response:", r.text)

        time.sleep(3)

    except Exception as e:
        print("WhatsApp Error:", e)

# ================= MAIN LOOP =================
while True:

    try:

        data = arduino.readline().decode(errors='ignore').strip()

        if not data:
            continue

        print("Received:", data)

        values = data.split(',')

        if len(values) != 3:
            continue

        user_id = values[0].strip()

        # تأكيد ID 4 digits بدون لعب زيادة
        user_id = user_id.zfill(4)

        name = values[1].strip()
        status = values[2].strip()

        now = datetime.now()
        date = now.strftime("%d/%m/%Y")
        current_time = now.strftime("%H:%M:%S")

        # ================= SAVE CSV =================
        writer.writerow([user_id, name, date, current_time, status])
        file.flush()

        # ================= MESSAGE =================
        message = (
            "📌 Attendance System\n"
            "------------------------\n\n"
            f"👤 Name: {name}\n"
            f"🆔 ID: {user_id}\n"
            f"📊 Status: {status}\n"
            f"📅 Date: {date}\n"
            f"⏰ Time: {current_time}"
        )

        # ================= SEND =================
        if user_id in contacts:

            for phone in contacts[user_id]:
                send_whatsapp(phone, message)

        else:
            print("No phone linked to ID:", user_id)

    except Exception as e:
        print("Error:", e)
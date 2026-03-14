from flask import Flask, request, jsonify
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import csv
import os
from datetime import datetime

app = Flask(__name__)

# --- Configuration ---
LOG_FILE = "pms_data_log.csv"
# InfluxDB Settings (Adjust to your local setup)
INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "YOUR_INFLUXDB_TOKEN"
INFLUX_ORG = "YOUR_ORG"
INFLUX_BUCKET = "PMS_Data"

# Initialize InfluxDB Client
client_influx = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = client_influx.write_api(write_options=SYNCHRONOUS)

def save_to_csv(data):
    file_exists = os.path.isfile(LOG_FILE)
    with open(LOG_FILE, 'a', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=data.keys())
        if not file_exists: writer.writeheader()
        writer.writerow(data)

def write_to_influx(data):
    try:
        point = Point("power_stats") \
            .field("v_ac", float(data['v_ac'])) \
            .field("a_ac", float(data['a_ac'])) \
            .field("w_ac", float(data['w_ac'])) \
            .field("e_ac", float(data['e_ac'])) \
            .field("v_dc", float(data['v_dc'])) \
            .field("battery_pct", float(data['bat'])) \
            .time(datetime.utcnow(), WritePrecision.NS)
        
        write_api.write(bucket=INFLUX_BUCKET, record=point)
    except Exception as e:
        print(f"InfluxDB Error: {e}")

@app.route('/data', methods=['POST'])
def receive_data():
    payload = request.get_json()
    if payload:
        payload['server_timestamp'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{payload['server_timestamp']}] Syncing to InfluxDB & CSV...")
        
        save_to_csv(payload)
        write_to_influx(payload)
        
        return jsonify({"status": "success"}), 200
    return jsonify({"status": "error"}), 400

if __name__ == '__main__':
    print("--- PMS Bapuk Server with InfluxDB Integration Active ---")
    app.run(host='0.0.0.0', port=5000)
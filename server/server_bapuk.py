from flask import Flask, request, jsonify
import csv
from datetime import datetime
import os

app = Flask(__name__)

# Simple Database File (CSV)
LOG_FILE = "pms_data_log.csv"

def save_to_csv(data):
    """Saves incoming JSON data to a CSV file."""
    file_exists = os.path.isfile(LOG_FILE)
    with open(LOG_FILE, 'a', newline='') as f:
        # Use JSON keys as CSV headers
        writer = csv.DictWriter(f, fieldnames=data.keys())
        if not file_exists:
            writer.writeheader()
        writer.writerow(data)

@app.route('/data', methods=['POST'])
def receive_data():
    """Endpoint for ESP32 to POST power monitoring data."""
    try:
        # Capture JSON payload from ESP32
        payload = request.get_json()
        
        if payload:
            # Add server-side timestamp for accurate logging
            payload['server_timestamp'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Print to console for real-time monitoring
            print(f"[{payload['server_timestamp']}] Data Inbound:")
            print(f"  AC System: {payload.get('v_ac')}V | {payload.get('w_ac')}W | {payload.get('e_ac')}kWh")
            print(f"  DC System: {payload.get('v_dc')}V | Battery: {payload.get('bat')}%")
            print(f"  System Uptime: {payload.get('uptime')}")
            print("-" * 45)
            
            # Save to CSV database
            save_to_csv(payload)
            
            return jsonify({
                "status": "success", 
                "message": "Data synchronized to local server"
            }), 200
        else:
            return jsonify({
                "status": "error", 
                "message": "Empty payload received"
            }), 400
            
    except Exception as e:
        print(f"Error processing request: {e}")
        return jsonify({
            "status": "error", 
            "message": str(e)
        }), 500

if __name__ == '__main__':
    # Flask runs on all interfaces (0.0.0.0) at port 5000
    # Ensure your laptop is connected to ESP32 AP and has IP 192.168.4.2
    print("===========================================")
    print("   PMS LOCAL SERVER v1.1 - ACTIVE          ")
    print("   Listening on http://0.0.0.0:5000/data   ")
    print("===========================================")
    app.run(host='0.0.0.0', port=5000, debug=True)
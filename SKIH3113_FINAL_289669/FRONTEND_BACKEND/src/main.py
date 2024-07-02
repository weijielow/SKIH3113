import json

from flask import Flask, jsonify, request
from flask_mqtt import Mqtt
from flask_mysqldb import MySQL
from datetime import datetime
from flask_cors import CORS
from collections import defaultdict
import statistics

app = Flask(__name__)
app.config['SECRET'] = 'SDKFJSDFOWEIOF'
app.config['TEMPLATES_AUTO_RELOAD'] = True
app.config['MQTT_BROKER_URL'] = '152.42.250.12'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 5
app.config['MQTT_TLS_ENABLED'] = False
app.config['MQTT_CLEAN_SESSION'] = True
app.config['MYSQL_HOST'] = 'localhost'
app.config['MYSQL_USER'] = 'root'
app.config['MYSQL_PASSWORD'] = '123456'
app.config['MYSQL_DB'] = 'skih3113_midterm'

mqtt = Mqtt(app)
mysql = MySQL(app)
CORS(app)

latest_config = {}
overall_stats = {}

@app.route('/')
def index():
    return "true"

@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    mqtt.subscribe('289669_store')

@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global latest_config
    now = datetime.now()
    current_date = now.strftime('%Y-%m-%d')
    current_time = now.strftime('%H:%M:%S')
    data = dict(
        topic=message.topic,
        payload=message.payload.decode()
    )
    print(data["payload"])
    json_data = json.loads(data['payload'])
    with app.app_context():
        cur = mysql.connection.cursor()
        cur.execute("INSERT INTO input (date, time, temperature, humidity, co2 ) VALUES (%s, %s, %s, %s,%s )",(current_date, current_time, json_data['temp'], json_data['humi'], json_data['concentration']))
        mysql.connection.commit()
        cur.close()

    latest_config = {
        'ssid': json_data['ssid'],
        'password': json_data['password'],
        'deviceID': json_data['deviceID'],
        'relayState': json_data['relayState'],
        'tempThreshold': json_data['tempThreshold'],
        'humiThreshold': json_data['humiThreshold'],
        'co2Threshold': json_data['co2Threshold'],
        'temperature': json_data['temp'],
        'humidity': json_data['humi'],
        'co2': json_data['concentration']
    }

@app.route('/api/data', methods=['GET'])
def get_data():
    with app.app_context():
        cur = mysql.connection.cursor()
        cur.execute("SELECT * FROM input ORDER BY date DESC, time DESC")
        rows = cur.fetchall()
        cur.close()

        # Assuming your table columns are (id, date, time, temperature, humidity, co2)
        data = []
        dates = defaultdict(list)
        for row in rows:
            data.append({
                'date': str(row[0]),
                'time': str(row[1]),
                'temperature': row[2],
                'humidity': row[3],
                'co2': row[4]
            })
            dates[str(row[0])].append({
                'temperature': row[2],
                'humidity': row[3],
                'co2': row[4]
            })

        global overall_stats
        for date, readings in dates.items():
            temperatures = [reading['temperature'] for reading in readings]
            humidities = [reading['humidity'] for reading in readings]
            co2_levels = [reading['co2'] for reading in readings]

            overall_stats[date] = {
                'min_temperature': min(temperatures),
                'max_temperature': max(temperatures),
                'avg_temperature': statistics.mean(temperatures) if temperatures else None,
                'min_humidity': min(humidities),
                'max_humidity': max(humidities),
                'avg_humidity': statistics.mean(humidities) if humidities else None,
                'min_co2': min(co2_levels),
                'max_co2': max(co2_levels),
                'avg_co2': statistics.mean(co2_levels) if co2_levels else None,
            }

    return {'data': data, 'overall_stats': overall_stats}


@app.route('/api/currentConfig', methods=['GET'])
def get_current_config():
    now = datetime.now()
    current_date = now.strftime('%Y-%m-%d')

    # Retrieve the averages for the current date
    daily_averages = overall_stats.get(current_date, {})

    notification = ""
    if daily_averages:
        current_temp = latest_config['temperature']
        current_humi = latest_config['humidity']
        current_co2 = latest_config['co2']

        if current_temp > daily_averages.get('avg_temperature', float('inf')):
            notification += "Current temperature is higher than today's average.\n"
        if current_humi > daily_averages.get('avg_humidity', float('inf')):
            notification += "Current humidity is higher than today's average.\n"
        if current_co2 > daily_averages.get('avg_co2', float('inf')):
            notification += "Current CO2 concentration is higher than today's average."
    print(notification)
    response = latest_config.copy()
    response['notification'] = notification

    return jsonify(response)


@app.route('/api/updateConfig', methods=['POST'])
def update_config():
    data = request.get_json()

    # Filter the required fields
    required_fields = ['co2Threshold', 'humiThreshold', 'tempThreshold', 'relayState', 'manualRelayControl']
    filtered_data = {key: data[key] for key in required_fields if key in data}

    # Publish the filtered configuration to the MQTT broker
    mqtt.publish('289669_update', json.dumps(filtered_data))

    return jsonify({"status": "success", "message": "Configuration updated successfully"}), 200

@mqtt.on_log()
def handle_logging(client, userdata, level, buf):
    print(level, buf)


if __name__ == '__main__':
    # important: Do not use reloader because this will create two Flask instances.
    # Flask-MQTT only supports running with one instance
    app.run(host='0.0.0.0', port=5000, use_reloader=False, debug=False)
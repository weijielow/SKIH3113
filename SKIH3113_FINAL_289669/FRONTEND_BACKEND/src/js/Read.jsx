import React, { useEffect, useState } from 'react';
import { ToastContainer, toast } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.css';
import './css/Read.css';

const Read = () => {
    const [currentConfig, setCurrentConfig] = useState({
        ssid: '',
        password: '',
        deviceID: '',
        relayState: false,
        tempThreshold: '',
        humiThreshold: '',
        co2Threshold: '',
        temperature: '',
        humidity: '',
        co2: '',
        manualRelayControl: false,
        notification: '' // Add notification field
    });

    const [formConfig, setFormConfig] = useState({
        tempThreshold: '',
        humiThreshold: '',
        co2Threshold: ''
    });

    useEffect(() => {
        fetchCurrentConfig();  // Fetch initial configuration when component mounts

        // Set up interval to fetch updated configuration every 5 seconds (adjust as needed)
        const interval = setInterval(() => {
            fetchCurrentConfig();
        }, 1000);  // 5000 milliseconds = 5 seconds

        // Clean up interval on component unmount
        return () => clearInterval(interval);
    }, []);

    useEffect(() => {
        if (currentConfig.notification) {
            toast(currentConfig.notification);
        }
    }, [currentConfig.notification]);

    const fetchCurrentConfig = async () => {
        try {
            const response = await fetch('http://127.0.0.1:5000/api/currentConfig');
            if (!response.ok) {
                throw new Error('Failed to fetch current configuration');
            }
            const data = await response.json();
            console.log("DEBUG: Fetched config data:", data);
            setCurrentConfig(data);
        } catch (error) {
            console.error('Error fetching current configuration:', error);
        }
    };

    const handleInputChange = (e) => {
        const { name, value } = e.target;
        setFormConfig(prevState => ({
            ...prevState,
            [name]: value
        }));
    };

    const handleSubmit = async (e) => {
        e.preventDefault();
        const updatedConfig = {
            ...currentConfig,
            ...formConfig
        };
        try {
            const response = await fetch('http://127.0.0.1:5000/api/updateConfig', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(updatedConfig)
            });
            if (!response.ok) {
                throw new Error('Failed to update configuration');
            }
            const data = await response.json();
            console.log("DEBUG: Updated config data:", data);
        } catch (error) {
            console.error('Error updating configuration:', error);
        }
    };

    const handleRelayToggle = async () => {
        const updatedConfig = {
            ...currentConfig,
            relayState: !currentConfig.relayState,
            manualRelayControl: true // Set manual control to true
        };
        setCurrentConfig(updatedConfig);
        try {
            const response = await fetch('http://127.0.0.1:5000/api/updateConfig', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(updatedConfig)
            });
            if (!response.ok) {
                throw new Error('Failed to update configuration');
            }
            const data = await response.json();
            console.log("DEBUG: Updated config data:", data);
        } catch (error) {
            console.error('Error updating configuration:', error);
        }
    };

    return (
        <div>
            <h1>Tracking Server</h1>
            <p>Welcome back, {currentConfig.deviceID}</p>
            <p>Current SSID: {currentConfig.ssid}</p>

            <div className="container">
                <div className="header">Current Value</div>
                <div className="content">
                    <div className="row"><p>Temperature:</p><p>
                        <span>{parseFloat(currentConfig.temperature).toFixed(2)}°C</span></p></div>
                    <div className="row"><p>Humidity:</p><p><span>{currentConfig.humidity}%</span></p></div>
                    <div className="row"><p>CO2 Concentration:</p><p>
                        <span>{parseFloat(currentConfig.co2).toFixed(2)}%</span></p></div>
                </div>
            </div>

            <div className="container">
                <div className="header">Current Threshold</div>
                <div className="content">
                    <div className="row"><p>Temperature:</p><p><span>{currentConfig.tempThreshold}°C</span></p></div>
                    <div className="row"><p>Humidity:</p><p><span>{currentConfig.humiThreshold}%</span></p></div>
                    <div className="row"><p>Co2:</p><p><span>{currentConfig.co2Threshold}%</span></p></div>
                    <form onSubmit={handleSubmit}>
                        <div>
                            <label>
                                Temperature Threshold:
                                <input
                                    type="text"
                                    name="tempThreshold"
                                    value={formConfig.tempThreshold}
                                    onChange={handleInputChange}
                                    placeholder="Enter temperature threshold"
                                    defaultValue={formConfig.tempThreshold}
                                />
                            </label>
                        </div>
                        <div>
                            <label>
                                Humidity Threshold:
                                <input
                                    type="text"
                                    name="humiThreshold"
                                    value={formConfig.humiThreshold}
                                    onChange={handleInputChange}
                                    placeholder="Enter humidity threshold"
                                    defaultValue={formConfig.humiThreshold}
                                />
                            </label>
                        </div>
                        <div>
                            <label>
                                Co2 Threshold:
                                <input
                                    type="text"
                                    name="co2Threshold"
                                    value={formConfig.co2Threshold}
                                    onChange={handleInputChange}
                                    placeholder="Enter Co2 concentration threshold"
                                    defaultValue={formConfig.co2Threshold}
                                />
                            </label>
                        </div>
                        <button type="submit">Update Thresholds</button>
                    </form>
                </div>
            </div>

            <div className="container">
                <div className="header">Relay Status</div>
                <div className="content">
                    <div className="row"><p>Status:</p><p><span>{currentConfig.relayState ? "ON" : "OFF"}</span></p>
                    </div>
                    <button onClick={handleRelayToggle}>
                        {currentConfig.relayState ? 'Turn Relay Off' : 'Turn Relay On'}
                    </button>
                </div>
            </div>

            <ToastContainer/>
        </div>
    );
};

export default Read;

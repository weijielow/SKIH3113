import React, { useEffect, useState } from 'react';
import { Table, Form, Container, Row, Col } from 'react-bootstrap';
import { LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip } from 'recharts';
import 'bootstrap/dist/css/bootstrap.min.css';

const Display = () => {
    const [data, setData] = useState([]);
    const [filteredData, setFilteredData] = useState([]);
    const [filterDate, setFilterDate] = useState('');
    const [overallStats, setOverallStats] = useState({});
    const [selectedDataType, setSelectedDataType] = useState('avg');

    useEffect(() => {
        fetch('http://127.0.0.1:5000/api/data')
            .then(response => response.json())
            .then(data => {
                setData(data.data);
                setFilteredData(data.data);
                setOverallStats(data.overall_stats);
            })
            .catch(error => console.error('Error fetching data:', error));
    }, []);

    const handleDateChange = (e) => {
        const date = e.target.value;
        setFilterDate(date);
        if (date) {
            const filtered = data.filter(row => row.date === date);
            setFilteredData(filtered);
        } else {
            setFilteredData(data);
        }
    };

    const handleDataTypeChange = (e) => {
        setSelectedDataType(e.target.value);
    };

    const getGraphData = () => {
        return Object.entries(overallStats).map(([date, stats]) => ({
            date,
            temperature: stats[`${selectedDataType}_temperature`],
            humidity: stats[`${selectedDataType}_humidity`],
            co2: stats[`${selectedDataType}_co2`]
        }));
    };

    return (
        <Container>
            <Row className="my-4">
                <Col>
                    <h2>Statistics</h2>
                    <h3>Data table</h3>
                    <Table striped bordered hover style={{borderColor: '#00a6b6'}}>
                        <thead style={{backgroundColor: '#00a6b6', color: '#fff'}}>
                        <tr>
                            <th>Date</th>
                            <th>Min Temperature</th>
                            <th>Max Temperature</th>
                            <th>Avg Temperature</th>
                            <th>Min Humidity</th>
                            <th>Max Humidity</th>
                            <th>Avg Humidity</th>
                            <th>Min CO2</th>
                            <th>Max CO2</th>
                            <th>Avg CO2</th>
                        </tr>
                        </thead>
                        <tbody>
                        {Object.entries(overallStats).map(([date, stats], index) => (
                            <tr key={index}>
                                <td>{date}</td>
                                <td>{stats.min_temperature}</td>
                                <td>{stats.max_temperature}</td>
                                <td>{stats.avg_temperature.toFixed(2)}</td>
                                <td>{stats.min_humidity}</td>
                                <td>{stats.max_humidity}</td>
                                <td>{stats.avg_humidity.toFixed(2)}</td>
                                <td>{stats.min_co2}</td>
                                <td>{stats.max_co2}</td>
                                <td>{stats.avg_co2.toFixed(2)}</td>
                            </tr>
                        ))}
                        </tbody>
                    </Table>
                </Col>
            </Row>

            <Row className="my-4">
                <Col>
                    <h3>Data Graph</h3>
                    <Form.Group controlId="dataTypeSelect">
                        <Form.Label>Select Data Type</Form.Label>
                        <Form.Control as="select" value={selectedDataType} onChange={handleDataTypeChange}>
                            <option value="min">Minimum</option>
                            <option value="max">Maximum</option>
                            <option value="avg">Average</option>
                        </Form.Control>
                    </Form.Group>
                    <LineChart width={900} height={300} data={getGraphData()}>
                        <Line type="monotone" dataKey="temperature" stroke="#8884d8" />
                        <Line type="monotone" dataKey="humidity" stroke="#82ca9d" />
                        <Line type="monotone" dataKey="co2" stroke="#ff7300" />
                        <CartesianGrid stroke="#ccc" />
                        <XAxis dataKey="date" />
                        <YAxis />
                        <Tooltip />
                    </LineChart>
                </Col>
            </Row>

            <Row className="my-4">
                <Col>
                    <h2>Overall Data</h2>
                    <h3>Data Graph</h3>
                    <LineChart width={900} height={300} data={filteredData}>
                        <Line type="monotone" dataKey="temperature" stroke="#8884d8"/>
                        <Line type="monotone" dataKey="humidity" stroke="#82ca9d"/>
                        <Line type="monotone" dataKey="co2" stroke="#ff7300"/>
                        <CartesianGrid stroke="#ccc"/>
                        <XAxis dataKey="time"/>
                        <YAxis/>
                        <Tooltip/>
                    </LineChart>
                </Col>
            </Row>

            <Row className="my-4">
                <Col>
                    <h3>Data Table</h3>
                    <Form.Group controlId="filterDate">
                        <Form.Label>Filter by Date</Form.Label>
                        <Form.Control
                            type="date"
                            value={filterDate}
                            onChange={handleDateChange}
                            placeholder="yyyy-mm-dd"
                        />
                    </Form.Group>
                    <Table striped bordered hover style={{ borderColor: '#00a6b6' }}>
                        <thead style={{ backgroundColor: '#00a6b6', color: '#fff' }}>
                            <tr>
                                <th>Date</th>
                                <th>Time</th>
                                <th>Temperature</th>
                                <th>Humidity</th>
                                <th>CO2</th>
                            </tr>
                        </thead>
                        <tbody>
                            {filteredData.map((row, index) => (
                                <tr key={index}>
                                    <td>{row.date}</td>
                                    <td>{row.time}</td>
                                    <td>{row.temperature}</td>
                                    <td>{row.humidity}</td>
                                    <td>{row.co2}</td>
                                </tr>
                            ))}
                        </tbody>
                    </Table>
                </Col>
            </Row>
        </Container>
    );
};

export default Display;

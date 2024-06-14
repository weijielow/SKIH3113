<?php
$servername = "localhost"; // Change if your database is hosted elsewhere
$username = "root"; // Your MySQL username
$password = "123456"; // Your MySQL password
$dbname = "skih3113_midterm"; // Your database name
date_default_timezone_set('Asia/Kuala_Lumpur');
// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Get the CO2 concentration, temperature, humidity, and timestamp from the GET request
if (isset($_GET['co2']) && isset($_GET['temp']) && isset($_GET['hum'])) {
    $co2 = $_GET['co2'];
    $temp = $_GET['temp'];
    $hum = $_GET['hum'];
    $date = date("Y-m-d");
    $time = date("H:i:s");

    // SQL query to insert data into the table
    $sql = "INSERT INTO input (date, time, co2, temperature, humidity) VALUES ('$date', '$time', '$co2', '$temp', '$hum')";

    if ($conn->query($sql) === TRUE) {
        echo "New record created successfully";
    } else {
        echo "Error: " . $sql . "<br>" . $conn->error;
    }
} else {
    echo "No data received";
}

$conn->close();
?>
<?php
$servername = "localhost";
$username = "root";
$password = "123456";
$dbname = "skih3113_midterm";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Fetch data from database
$sql = "SELECT date, time, co2, temperature, humidity FROM input ORDER BY date DESC, time DESC";
$result = $conn->query($sql);

$data = array();
if ($result->num_rows > 0) {
    // Output data of each row
    while($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
} else {
    echo "0 results";
}
$conn->close();

// Return data as JSON
echo json_encode($data);
?>
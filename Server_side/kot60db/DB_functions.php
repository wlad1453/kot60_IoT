<?php

require 'kot60db/kot60credentials.php';

$conn = db_connect($cred_db);
// db_entry($conn);
// read_db($conn);

function db_connect(&$cred) { //Passing arguments by reference
	// Create connection
	$conn = new mysqli($cred['server'], $cred['user'], $cred['pwd'], $cred['db']); 

	// Check connection
	if ($conn->connect_error) die("Connection failed: " . $conn->connect_error); 
	echo "Connected successfully<br>";	

	return $conn;
}

// require 'heatData.php'; 
// $H_data = array ("Ut"=>$Ut, "Um"=>$Um, "Bm"=>$Bm, "Bt"=>$Bt, "kitT"=>$kitT, "livT"=>$livT, "bathT"=>$bathT, "corrT"=>$corrT, "bedT"=>$bedT, "TstT"=>$TstT);
	/*	$Ut = 151; $Um = 70; $Bm = 50; $Bt = 30; $kitT = 26; $livT = 21; $bathT = 27; $corrT = 19; $bedT = 17; $TstT = 36.6;
		DB: R_KitT	decimal(5,2), R_LivT decimal(5,2), R_BedT decimal(5,2), R_KorT decimal(5,2), R_BathT double(5,2), R_HrT*/
	
function db_entry(&$conn, &$H_data) {
	
	// require 'heatData.php'; 
		/*$Ut = 151; $Um = 70; $Bm = 50; $Bt = 30; $kitT = 26; $livT = 21; $bathT = 27; $corrT = 19; $bedT = 17; $TstT = 36.6;
		 DB: R_KitT	decimal(5,2), R_LivT decimal(5,2), R_BedT decimal(5,2), R_KorT decimal(5,2), R_BathT double(5,2), R_HrT*/

	date_default_timezone_set("Europe/Moscow");  
	$DateTime = date("Y-m-d h:i:s");
	
	$sql = "INSERT INTO Temp_sensors (DateTime, HA_U, HA_Um, HA_Bm, HA_B, R_KitT, R_LivT, R_BedT, R_KorT, R_BathT)
	VALUES ('" . $DateTime . "', " . $H_data['Ut'] . ", " . $H_data['Um'] . ", " . $H_data['Bm'] . ", " . $H_data['Bt'] . ", "
	. $H_data['kitT'] . ", " . $H_data['livT'] . ", " . $H_data['TstT'] . ", " . $H_data['corrT'] . ", " . $H_data['bathT'] . ")";
	
	// echo $sql . "<br>";

	if ($conn->query($sql) === TRUE) {
	  echo "New record created successfully at " . $DateTime . "<br><br>" ;
	} else {
	  echo "Error: " . $sql . "<br>" . $conn->error;
	}
	
	$conn->close();
}

function read_db(&$conn) {
	// Reading DB table

	$sql = "SELECT Measmt_ID, DateTime, HA_U, HA_Um, HA_Bm, HA_B, R_KitT, R_LivT, R_BedT, R_KorT, R_BathT FROM Temp_sensors";
	$result = $conn->query($sql);

	echo "<table>
	<tr>
	<th>ID</th> <th>DateTime</th> <th>U</th> <th>Um</th> <th>Bm</th> <th>B</th>
	<th>Kitch.</th> <th>Living</th> <th> Bedr.</th> <th> Corr.</th> <th> Bath </th>
	</tr>";

	if ($result->num_rows > 0) {
	  // output data of each row
	  while($row = $result->fetch_assoc()) {
		  
		echo "<tr>";
			echo "<td>" . $row["Measmt_ID"] . "</td>";	echo "<td>" . $row["DateTime"] . "</td>";
			echo "<td>" . $row["HA_U"] . "</td>"; 		echo "<td>" . $row["HA_Um"] . "</td>";		echo "<td>" . $row["HA_Bm"] . "</td>";		echo "<td>" . $row["HA_B"] . "</td>";
			echo "<td>" . $row["R_KitT"] . "</td>"; 	echo "<td>" . $row["R_LivT"] . "</td>";		echo "<td>" . $row["R_BedT"] . "</td>";		echo "<td>" . $row["R_KorT"] . "</td>"; 
			echo "<td>" . $row["R_BathT"] . "</td>";
		echo "</tr>";
	  }
	} else {
	  echo "0 results";
	}
	
	$conn->close();
}

// $conn->close();
 
?>
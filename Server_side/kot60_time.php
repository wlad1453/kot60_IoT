<?php

date_default_timezone_set("Europe/Moscow");  
	
	include 'heatData.php';
	
	$sysT = date("H:i:s");			/* system time H, i, s */
	$sysD = date("d/m/y");
	

?>

Syst. time:    <?php echo $sysT . ",   " . $sysD . " eqw." . $eqw;?><br>


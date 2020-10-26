<!DOCTYPE html>
<html>
  <head>
    <title>House plan with room T°</title>
	<style>
	body { background-color: #c4e3ed;
	font-family: arial;
	}
	.button {
	  display: inline-block;
	  width: 55px;
      height: 33px;		  
	  padding: 3px 5px;
	  font-size: 18px;
	  margin: 4px 2px;
	  cursor: pointer;
	  text-align: center;
	  text-decoration: none;	  
	  outline: none; 
	  color: white;
	  background-color: #B0B0BB;
	  border: 3px solid #535377;
	  box-shadow: 0 4px #999;
	  -webkit-transition-duration: 0.4s; /* Safari */
	  transition-duration: 0.4s;
	}
	.button:hover {
	  border: 3px solid #535377;
	  opacity: 0.7;
	  color: white;
	}
	.button:active{
		background-color: #CC3030;
		box-shadow: 0 3px #666;
		transform: translateY(3px);
	}
	.barOn{
		background-color: #EE1010;
	}
	.barOff{
		background-color: #3030EE;
	}
	.sync{	
		background-color: #40AA40;	
		width: 365px;
	}
	
	</style>
  </head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
    
  <body>
  
	<h2 id = "caption" align="center" style="font-family: arial;">Cottage 60</h2>
	
	<div id = "time" align="center" style="font-family: arial; font-weight: normal; font-size: 20px;">	</div>
 	<div id = "A6time" align="center" style="font-family: arial; font-size: 20px;">	  </div>
	
	<div id="houseSVG" align="center" w3-include-html="kot60pic.html">svg picture</div>
	
	<form action="login_form.php" method="post" align="center">
	  
	  <button type="submit" class="button sync" formaction="appScript.php">Heat accumumulator</button><br>
	  <button type="submit" class="button sync" formaction="kot60_DB.php">Heating data</button><br>
	  <button type="submit" class="button sync" >Setup</button>
	</form>
	
	</body>	
	
		<script type="text/javascript">	
		
		window.onload = function() { 			
			let t = setInterval(ServerExchange, 1000); 		
		}
		
		includeHTML();
		
		function includeHTML() {
		  var z, i, elmnt, file, xhttp;
		  
		  z = document.getElementsByTagName("*");        			/* Loop through a collection of all HTML elements: */
		  for (i = 0; i < z.length; i++) {
			elmnt = z[i];
			
			file = elmnt.getAttribute("w3-include-html");			/*search for elements with a certain atrribute: */
			if (file) {
			 
			  xhttp = new XMLHttpRequest(); 						/* Make an HTTP request using the attribute value as the file name: */
			  xhttp.onreadystatechange = function() {
				if (this.readyState == 4) {
					if (this.status == 200) {elmnt.innerHTML = this.responseText;}
					if (this.status == 404) {elmnt.innerHTML = "Page not found.";}
					
					elmnt.removeAttribute("w3-include-html");		/* Remove the attribute, and call this function once more: */
					includeHTML();
				}
			  }
			  xhttp.open("GET", file, true);
			  xhttp.send();
			  
			  return;		/* Exit the function: */
			}
		  }
		}	/* func. includeHTML() */		

			function getServerTime() {
				var xhttp = new XMLHttpRequest();
				xhttp.onreadystatechange = function() {
					if (this.readyState == 4 && this.status == 200) {
						document.getElementById("time").innerHTML = this.responseText;
					}
				};
				xhttp.open("GET", "kot60_time.php", true);  
				xhttp.send();
			}
			
			function getTemp1stFloor() {
				
				var xhttpT = new XMLHttpRequest();
				xhttpT.onreadystatechange = function() {

					if (this.readyState == 4 && this.status == 200) {
						tempDataParse(this.responseText); 						
					}
				};
				xhttpT.open("GET", "heatData.txt", true);  
				xhttpT.send();
			} 
			
			function tempDataParse(InitialStr = "") {
				measT = dataParse(InitialStr, "$measT", 10, 8);
				measD = dataParse(InitialStr, "$measD", 10, 8);
				Scenario = dataParse(InitialStr, "$Scenario", 13, 8); 
				
				kitT = dataParse(InitialStr, "$kitT", 8, 2);
				livT = dataParse(InitialStr, "$livT", 8, 2);
				bathT = dataParse(InitialStr, "$bathT");
				corrT = dataParse(InitialStr, "$corrT");
				TstT = dataParse(InitialStr, "$TstT", 8, 4); /* just to test float numbers */
				
				/* console.log("kitT: " + kitT + " livT: " + livT + " bathT: " + bathT + " corrT: " + corrT + " TstT: " + TstT); */
				
				/* document.getElementById("kitchenT").innerHTML = kitT + "°C"; */
				
				showTemp(measT, measD, Scenario, kitT, livT, bathT, corrT, TstT);
			} 
			
			function dataParse(InitialStr = "", searchStr = "", data1stPos = 8, dataLength = 3) {
				var pos = InitialStr.indexOf(searchStr) + data1stPos;
				return InitialStr.slice(pos, pos + dataLength);
			}
		
		function showTemp(measT, measD, Scenario, kitT, livT, bathT, corrT, TstT) {
			setTimeout(function f(){}, 250);
			
			document.getElementById("A6time").innerHTML = "ModemA6: " + measT + ", " + measD + " " + Scenario + "<br><br>";
			
			document.getElementById("kitchenT").innerHTML = kitT + "°C";
			document.getElementById("livingT").innerHTML = livT + "°C"; 
			document.getElementById("bathT").innerHTML = bathT + "°C"; 
			document.getElementById("corridorT").innerHTML = corrT + "°C"; 
			document.getElementById("bedroomT").innerHTML = TstT + "°";			/* bedT + "°C"; Test of float T value */
		}
				
		/* setTimeout(function f(){showTemp( kitT, livT, bathT, corrT, TstT )}, 250); */ /* <?php echo $livT . "," . $bathT . "," . $corrT . "," . $TstT ?>*/

		
		function ServerExchange() {
			getServerTime();
			getTemp1stFloor();			
		}
		
	</script>
</html>

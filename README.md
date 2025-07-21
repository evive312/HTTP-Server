
<h1>Simple HTTP server in C</h1>


<h2>Description</h2>
Project consists of a single‑threaded HTTP server in plain C that reads its port from port.txt, binds to 127.0.0.1, and handles a handful of built‑in routes plus file serving. Incoming requests are parsed into a fixed 4 KB buffer, then dispatched to     /ping (returns “pong”), /echo (mirrors the request body back), POST /write and GET /read (write to and read from a simple in‑memory buffer), or GET /<filename> (streams that file with correct Content-Length).

Errors (400, 404, 413) are handled with minimal headers, connections are closed cleanly, and everything—socket setup, request parsing, header formatting, file I/O—is done with POSIX calls and no external deps, making it a concise example of low‑level HTTP mechanics.****
<br />


<h2>Languages Used</h2>

- <b>C</b> 


<h2>Environments Used </h2>

- <b>Ubuntu 24</b>

<h2>Program walk-through:</h2>
<!--
<p align="center">
Launch the utility: <br/>
<img src="https://i.imgur.com/62TgaWL.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Select the disk:  <br/>
<img src="https://i.imgur.com/tcTyMUE.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Enter the number of passes: <br/>
<img src="https://i.imgur.com/nCIbXbg.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Confirm your selection:  <br/>
<img src="https://i.imgur.com/cdFHBiU.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Wait for process to complete (may take some time):  <br/>
<img src="https://i.imgur.com/JL945Ga.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Sanitization complete:  <br/>
<img src="https://i.imgur.com/K71yaM2.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
<br />
<br />
Observe the wiped disk:  <br/>
<img src="https://i.imgur.com/AeZkvFQ.png" height="80%" width="80%" alt="Disk Sanitization Steps"/>
</p>
--!>
<!--
 ```diff
- text in red
+ text in green
! text in orange
# text in gray
@@ text in purple (and bold)@@
```
--!>

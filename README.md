<span class="c9"></span>

<span align=""  class="c8">
<h1  align="center" class="c8">FOX-1 WATCH</h1>

<h6 align="center" class="c8">DIY Open Source AI Agent Watch
</h3>

<span style="overflow: hidden; display: inline-block; margin: 0.00px 0.00px; border: 0.00px solid #000000; transform: rotate(0.00rad) translateZ(0px); -webkit-transform: rotate(0.00rad) translateZ(0px); width: 624.00px; height: 356.00px;">![](/images/image3.png)</span>

<span class="c13"><strong>FOX1-WATCH(FOX-1W)</strong>  is a DIY Open Source AI Agent Watch project designed for tech enthusiasts to build their own AI-powered smartwatch from the off-the-shelved hardware component. The watch serves as a client to any AI Agent OS/Server that can communicate through web sockets and is built on  </span><span class="c11">[LMC Messages architecture](https://docs.openinterpreter.com/protocols/lmc-messages&sa=D&source=editors&ust=1716829156805021&usg=AOvVaw2tptlOOfMw8dtKrkPCzqrM)</span><span class="c1"> by Open Interpreter.</span>

<span class="c1">Currently, the programmable watch used is the Lilygo Esp-S3 watch. The watch connects to the Open Interpreter 01’s flagship operating system (“01OS”) that can power conversational, computer-operating AI devices similar to the Rabbit R1 or the Humane Pin.</span>

<span class="c1"></span>

<span class="c0"><strong>Feature:</strong></span>

- <span class="c1"><strong>AI Agent tasks execution on PC/Mac through the Open Interpreter 01OS. These include:</strong></span>

  - <span class="c1">Answer general questions</span>
  - <span class="c1">Note making</span>
  - <span class="c1">Create ToDo List</span>
  - <span class="c1">Play video/music on YouTube.</span>
  - <span class="c1">Wealth queries</span>
  - <span class="c1">Any other task Open Interpreter 01OS can do</span>

- <span class="c0"><strong>Other Functions(Native to watch)</strong></span>
  - <span class="c15 c13">Time</span>
  - <span class="c15 c13">Pedometer(Coming soon)</span>
  - <span class="c13 c15">Alarm/Reminders(Coming soon)</span>

<span class="c1"></span>

<span class="c0"><strong>Hardware:</strong></span>

- <span class="c1">[LILYGO® T-Watch-S3 Programmable Touchable Watch](https://bit.ly/4dUvlhL)</span>
- <span class="c17 c13">more hardwares coming soon</span>

<span class="c1">        </span>

<span class="c0"><strong>AI OS(Server):</strong></span>

- <span class="c11">[Open Interpreter 01OS](https://01.openinterpreter.com/getting-started/introduction&sa=D&source=editors&ust=1716829156806212&usg=AOvVaw3A8H8QwbRtWcFzhcbOz10l)</span>
- <span class="c13 c17">FOX- xOs(Coming Soon)</span>

<span class="c1">        </span>

<span class="c1"></span>

<span class="c0"><strong>Installation Guide.</strong></span>

<span class="c0"><strong>Hardware</strong></span>

1.  <span class="c1">Get yourself a [LILYGO® T-Watch-S3 Programmable Touchable Watch](https://bit.ly/4dUvlhL)</span>
2.  <span class="c1">Clone this repo into Arduino IDF</span>
3.  <span class="c13">Goto</span> <span class="c11">[LilyGO T-Watch](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library)</span><span class="c1"> to install the library for the watch and also setup the Arduino IDF environment.</span>
4.  <span class="c13">Install Esp32 Sketch Data Upload library and upload UI files to the</span> <span class="c20">SPIFFS</span><span class="c13">(follow this</span> <span class="c11">[blog post](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)</span><span class="c1">)</span>
5.  <span class="c1">Now flash the code in this repo to watch for the next steps.</span>

<span class="c1"></span>
<span class="c1"></span>
<span class="c0"><strong>Software(OS Server)</strong></span>

1.  <span class="c13">Goto:</span> <span class="c11">[Open Interpreter 01OS](https://01.openinterpreter.com/getting-started/introduction)</span><span class="c13">, and follow the instructions there to setup your AI OS server.</span>

<span class="c0">        </span>

<span class="c0"><strong>FOX-1 WATCH Configuration</strong></span>

<span class="c1">Assuming the hardware and OS server installation went well. You should see the home screen of the watch. Now let's go ahead and connect the watch to Wifi and the server.</span>

1.  <span class="c1">Open the WiFi Setting on your Phone or PC.</span>
2.  <span class="c1">Identify the WiFI with the name <i><strong>“FOX-1Watch”</strong></i> and connect to it.</span>
3.  <span class="c1">After a successful connection, a FOX-1Watch Setup Page will pop up.</span>
4.  <span class="c13">Tap/Click on</span> <span class="c0"><i><strong>“Get started”</strong></i></span>
5.  <span class="c13">On the</span> <span class="c13 c18">Connectivity Settings</span><span class="c1"> page, Select your local WiFi, enter your password, and tap/click <i><strong>“Connect”</strong></i>  .</span>

<span style="overflow: hidden; display: inline-block; margin: 0.00px 0.00px; border: 0.00px solid #000000; transform: rotate(0.00rad) translateZ(0px); -webkit-transform: rotate(0.00rad) translateZ(0px); width: 624.00px; height: 356.00px;">![](/images/image1.png)</span>

<span class="c1"></span>

6.  <span class="c1">If the Wi-Fi connection was successful, the OS Server settings page will pop up. Here select the OS from the list and enter the address in this format <strong>server_ip:10001</strong>, server_ip  is your PC’s IP.</span>

<span style="overflow: hidden; display: inline-block; margin: 0.00px 0.00px; border: 0.00px solid #000000; transform: rotate(0.00rad) translateZ(0px); -webkit-transform: rotate(0.00rad) translateZ(0px); width: 624.00px; height: 357.33px;">![](/images/image2.png)</span>

<span class="c1">                </span>

<span class="c1"></span>

7.  <span class="c1">If the server connection goes well, you should see something like whats the screenshot below. On the watch, a WiFi symbol will appear on the status bar.</span>
2.  <span class="c1">Now swipe up on the home screen of the watch, hold down the mic button talk, and release the mic button to get a response from the AI Agent.
<i><strong>😎 ENJOY</strong></i> 
</span>

<span style="overflow: hidden; display: inline-block; margin: 0.00px 0.00px; border: 0.00px solid #000000; transform: rotate(0.00rad) translateZ(0px); -webkit-transform: rotate(0.00rad) translateZ(0px); width: 624.00px; height: 356.00px;">![](/images/image4.png)</span>

<span class="c1"></span>

<span class="c13"></span>
<span class="c1">
<strong>ToDo List:</strong>
- <span class="c15 c13">Clean up code</span>
- <span class="c15 c13">FOX-x0s</span>
- <span class="c15 c13">Pedometer</span>
- <span class="c13 c15">Alarm/Reminders</span>
</span>
<span class="c1"></span>

<span class="c1"><strong>Contribution Request:</strong></span>
You can reach out [here](https://x.com/itsmrquansah/)
- <span class="c15 c13">I need someone to manage the repo's PRs</span>
</span>


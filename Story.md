# the why
I recently upgraded the setup for my potted plants. This included some nice lights and a proper shelf.

While I really enjoy looking at the plants, I am not so good at keeping them alive. I either overwater or underwater them, depending on my mood...
Since I like to dabble in electronics and also write code, why not build some "smart" stuff to keep track of the humidity level in the ground of my plants and notify me if I need to take action.

# the what
This got me motivated to finally start developing for some micro processors. In the past I already had started to do some simple programs in arduino (Arduino Uno R3), but for a smart sensor I'd like to have something that requires a minimal amount of power, can measure the humidity in the ground and send the values somehwere for analysis. 

# the plan
So the plan is to build a sensor unit, stick the sensor unit in the ground and have it report the humidity level to a central data collector. The datacollector then should notfiy me if the humidity goes out of a certain threshold, i.e. the plant gets too dry.
This would also allow me to further my knowledge on the C language, microcontrollers, network communcations, tiny bit of databases and data anylsis and also a tiny bit in electronics.

# the prototyping

## the sensor
I decided to build on the basis of ESP32s, as they have builtin ADC, network and can be configured to run on quite low power (if done properly).
After doing some research I decided to build everything on the [ESP-Now protocol](https://www.espressif.com/en/solutions/low-power-solutions/esp-now). It allows to send data between ESPs in a low power mode without the need to have them in the same network, no need for an accesspoint or a router. They can just be. And with proper configuration they don't require an echo and can just broadcast into the void. This reduces the time the ESPs need to be in wake-mode, reducing the power consumption. This is perfect if you don't want them to access your network, maybe want to add many many sensors or just want to be able to randomly add/remove them without them cluttering your wireless network. No separete VLANS, etc. they just work.

Getting ESP-now to run was easy at first. Just use the example provided by Espressif on [Github](https://github.com/espressif/esp-idf/tree/v5.2.1/examples/wifi/espnow). Then me being a total C/C++ N00b I wanted to send my own data, and not the random data they send in the example. 
It took me way to long to understand that the example is sending the entire send_params struct and not only the data in the struct (buffer), while only sending the the length of the buffer, and not the length of the entire send_param struct + buffer size:
```
esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len)
```

after modifying the data to send to point to the actual buffer payload, and not the entire buffer, I was finally able to send "my" own data across to a different ESP:
```
esp_now_send(send_param->dest_mac, ((example_espnow_data_t*)(send_param->buffer))->payload, send_param->len)
```
However this example is way too complex for me at the start, so I removed everything I did not need to get a minimal running example.

## the data collector
But I want to be able to access the data from a non-esp device to be able to store, the data. Luckily there were some smart people that had the same requirements as I do and wrote some sort of a driver with really nice explainations on it. In his [Hackaday Project](https://hackaday.io/project/161896-linux-espnow), Thomas Flayols is describing his journey to get the ESPs to communicate via ESP-now to linux. His use case communicating wirelessly with a robot from the controller. It's a great and very detailed read I can recommend!
I shamelessly copied their library from [here](https://github.com/thomasfla/Linux-ESPNOW/tree/master/ESPNOW_lib) to build my own stuff in it. Hey, they even included a main I can run to test my stuff with!
I got the program to output the data I sent from my ESPs!

But where do I run this on? My first idea was to run it on a BananaPI M2 zero, as I had one lying around. However a quick google revealed that a network interface running in monitor mode is not really able to send data to somehwere. So I'd either need a machine that has two wireless NICs, or a wired one together with a WIFI NIC.

Some brainstorming in TeamSpeak got me the idea to run this thing on my self-built NAS (UnRaid) in a Docker container and call it a day. This has some benefits:
- I don't need to run the BananaPI
- The data is already on my NAS for further use
- I can easily update the docker container to a newer version while not needing to worry about messing up the OS of my NAS.

So I built a docker container for my application and had some issues getting the docker network to be able to have direct access to the NIC. After meddling with the MACVLAN i settled on the HOST network. 
And now my application was also running in Docker, monitoring the network and giving me the same output as my machine! Time to move the docker to the NAS.

This is where the docker journey came to an end. Unraid does not support wifi, therefore does not have the drivers installed. And after a lot of googleing the only solution I found was to use a wifi->lan adapter in one form or another to have the wifi part handled by a 3rd party device and only route the ethernet cable to the UnRaid machine - not what I intended to do...

Unfortunately no Unraid Community App provides the drives for my NIC, and installing the drivers manually is also not option, as this would mean that I have to mess with the Unraid-OS - something I'd like to avoid. Also I'd most likely have to plug in the drivers or update the kernel after every UnRaid update... Not good/nice.

After further brainstorming I decided to run a slim linux-vm on one core, pass through the NIC (friendly reminder to have HVM and IOMMU enabled in the BIOS. I did not. At first.) and have my application running there. Not as nice as having it run in a docker, but better than having to service the banana pi.
Installing my app in a freshly built VM and setting up the WIFI NIC and auto starting the interface I was able to receive the same data my machine and my docker was receiving from the ESPs - success!

But why is the CPU usage at 100%? This got me to research for hours on hours on blocking, vs non blocking socket, polling vs selecting the sockets, and general networking in C.
Trying everything and throwing `sleep()`s around the CPU usage never went down. After more time than I'd like to admit, I finally noticed that the main-loop of the main thread did nothing. At full CPU capcacity.
Turns out, throwing the sleep at the correct position works...
But hey, at least I can now recommend  [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/).

Now that the CPU is idle most of the time and the VM is receiving everything it should, it is time to actually do something with the data.

## the do something with the data
Since I only recently (at the time of writing this) built the NAS, I only started meddling with collecting system data and displaying them. So I decided to use Prometheus for further collecting and Grafana for displaying the data, and also use the in Grafana builtin notification system to send me push messages.
However I quickly noticed, that Prometheus does not really play well with data that is from the moment it is scraping the data, but older data. Or even multiple data points of the same thing. Some research later I settled with an InfluxDB. I can run an InfluxDB in Docker on my NAS, send all the data from the data collector directly to the instance and let grafana do it's thing based on this data.

# the hardware
Finally time to talk about the hardware:

So I got my self some components (without proper research. Let's see if this causes any issues in the future...)
- esp32 
- battery
- solar panel

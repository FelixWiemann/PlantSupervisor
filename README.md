# Purpose

# Overview
Sensors are placed in the plants earth to measure the humidity of the ground

AccessPoint is the point where the Sensors will push their data to. AccessPoint can then be scraped by Prometheus and the data be displayed via Grafana


## AccessPoint
a `node.js` http server in a docker container accepting `POST` requests on `/plant` with JSON data in format of

``` JSON
{
    "plant_id": "testplant",
    "humidity": "5"
}
```
when data is put, the server will also add a timestamp. This is to avoid having to synchronize time across the sensors. 

It also provides an `GET` endpoint on `/metrics` to get all the stored data points.
The data will be provided in the format of 
```
plant_ground_humidity{plant_id=<plant_id>} <humidity_value> <timestamp>
```

example output:
```
plant_ground_humidity{plant_id="testplant"} 6 1716208189255
plant_ground_humidity{plant_id="testplant"} 5 1716208189478
plant_ground_humidity{plant_id="testplant"} 5 1716208189645
plant_ground_humidity{plant_id="testplant"} 4 1716208189811
plant_ground_humidity{plant_id="testplant"} 3 1716208189962
plant_ground_humidity{plant_id="testplant"} 3 1716208190095
```


After the data has been gathered, the endpoint will delete all stored data.
The data is not persisted across reboots.


## protocol versions:

size |  Version  | values 
 3   |    N/A    | humidity value => len3: MSB, LSB, 0
 4   | 0x00 0x01 | humidity value => len2: MSB, LSB 
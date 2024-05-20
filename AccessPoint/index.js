const axios = require('axios')
const http = require('http')

data = []

handlePost = (req, res) => {
    if (req.url==="/plant") {
        try{
            res.writeHead(200,{'Content-Type': 'text/plain'})
            req.on("data", function(chunk) {
                d = JSON.parse(chunk.toString())
                if (d.plant_id === undefined) {
                    console.error("plant id missing")
                    return
                }
                if (d.humidity === undefined) {
                    console.error(`humidity missing for plant ${d.plant_id}`)
                    return
                }
                data.push({plant_id: d.plant_id, humidity: d.humidity, timestamp: Date.now()})
            });
            res.end()

        }
        catch (error) {
            console.error("got error processing plant " + error)
            
        }
        return
    }
    console.log("got invalid post request on " + req.url)
}

handleGet = (req, res) => {
    if (req.url==="/metrics") {
        try{

            res.writeHead(200,{'Content-Type': 'text/plain'})
            data.forEach(element => {
                res.write(`plant_ground_humidity{plant_id="${element.plant_id}"} ${element.humidity} ${element.timestamp}\n`)
            });
            res.end()
            data = []
        } catch (error) {
            console.error("got error processing metrics " + error)
        }
        return
    }
    console.log("got invalid get request on " + req.url)
}

handleError = (req, res) => {
    res.writeHead(400,{'Content-Type': 'text/plain'})
    console.error("got invalid request type on " + req.url)
    res.write("error")
    res.end()
}


http.createServer((req, res) => {

    switch (req.method) {
        case "POST":
            handlePost(req, res)
            break
        case "GET":
            handleGet(req, res)
            break
        default:
            handleError(req, res)
            break
    }

}).listen(4116)
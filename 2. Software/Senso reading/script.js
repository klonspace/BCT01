fetch("./posdata.txt").then(body => body.text()).then(body => {
    var lines = body.split("\n")
    console.log(lines);
    var prev = 0;
    lines.forEach((line, i) => {
        var val = parseFloat(line);
        if (val < prev) {
            console.log("value decreased at line :" + i + " val : " + val)
        }
        prev = val;
    })
    // var previndex = 0;
    // lines.forEach((line, i) => {
    //     var val = parseFloat(line);
    //     if (val > prev) {
    //         console.log("value increased at line :" + i + " val : " + val)
    //         console.log("interval : " + (i - previndex))
    //         prev = val;
    //         previndex = i;
    //     }
    // })
})
var treeData;

var width = 500;
var margin = {top: 10, right: 10, bottom: 10, left: 10};
var treeWidth = 500;
var treeHeight = 400;

function getData(callback) {
    window.profiler.getCSV(function(csv) {
        console.warn(csv);
        var rows = d3.csv.parse(csv);

        console.warn(rows);
        console.warn("parsed " + rows.length + " rows");

        objectiveDomain = "cost";

        var data = rows;
        data[data.length] = {id: -1, parentId: -(data.length*2), visId: 0, root: true, timetaken: 0, restartId: 0}
        data.forEach(type);
        data.sort(function(a,b) {return a.id-b.id;});
        callback(data);
    });
}

function initialise(obj) {
    var body = document.getElementsByTagName("body")[0];
    console.warn("initialise OK");

    var csv = obj.s;
    var rows = d3.csv.parse(csv);

    console.warn(rows);

    console.warn("parsed " + rows.length + " rows");

    objectiveDomain = "cost";

    var data = rows;
    data[data.length] = {id: -1, parentId: -(data.length*2), visId: 0, root: true, timetaken: 0, restartId: 0} // add in fake root!
    data.forEach(type);
    data.sort(function(a,b) {return a.id-b.id;}); // make it the first one..
    treeData = nestData(data)[0];

//    drawTimeLine(treeData, data);
    drawAggregated(data, 1);
    drawIcicle(treeData);

    new QWebChannel(qt.webChannelTransport, function(channel) {
        console.warn("channel here");
        window.message = channel.objects.messenger.message;
//        channel.objects.messenger.message("hello, world");
    });

    return "OK";
}

function type(d) {
  d.id = +d.id;
  d.parentId = +d.parentId;
  d.status = +d.status;
  d.alternative = +d.alternative;
  d.decisionLevel = +d.decisionLevel;
  d.depth = +d.depth;
//  d.label = d.label;
  d.subtreeDepth = +d.subtreeDepth;
  d.subtreeSolutions = +d.subtreeSolutions;
  d.subtreeSize = +d.subtreeSize;
  d.nogoodStringLength = +d.nogoodStringLength;
  d.nogoodString = d.nogoodString;
  d.nogoodLength = +d.nogoodLength;
  d.nogoodNumberVariables = +d.nogoodNumberVariables;
  d.backjumpDistance = +d.backjumpDistance;
  d.timestamp = +d.timestamp;
//  d.solutionString = d.solutionString;
  return d;
}

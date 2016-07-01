var treeData;

var width = 500;
var margin = {top: 10, right: 10, bottom: 10, left: 10};
var treeWidth = 500;
var treeHeight = 400;

function getData(callback) {
    var csv = window.profiler.getCSV();
    // window.profiler.getCSV(function(csv) {
    var rows = d3.csv.parse(csv);
    
    objectiveDomain = "cost";
    
    var data = rows;
    data[data.length] = {id: -1, gid: -1, parentId: -(data.length*2), visId: 0, root: true, timetaken: 0, restartId: 0}
    data.forEach(type);
    data.forEach(getVariables);
    data.sort(function(a,b) {return a.id-b.id;});

    variableListString = window.profiler.getVariableListString();

    callback(data, variableListString);
    // });
}

function initialise(obj) {
    var body = document.getElementsByTagName("body")[0];

    var csv = obj.s;
    var rows = d3.csv.parse(csv);

    objectiveDomain = "cost";

    var data = rows;
    data[data.length] = {id: -1, gid: -1, parentId: -(data.length*2), visId: 0, root: true, timetaken: 0, restartId: 0} // add in fake root!
    data.forEach(type);
    data.sort(function(a,b) {return a.id-b.id;}); // make it the first one..
    treeData = nestData(data)[0];

//    drawTimeLine(treeData, data);
    drawAggregated(data, 1);
    drawIcicle(treeData);

    new QWebChannel(qt.webChannelTransport, function(channel) {
        window.message = channel.objects.messenger.message;
//        channel.objects.messenger.message("hello, world");
    });

    return "OK";
}

function type(d) {
  d.id = +d.id;
  d.gid = +d.gid;
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


function getVariables(data) {
  if (typeof data.solutionString != "undefined" && data.solutionString != "" )
  {
    data.solutionObject = JSON.parse(data.solutionString);
  }
    data.variable = data.label;
    data.variableGroup = "NA";
   if (typeof data.variable == "undefined" | data.variable == "")
   {data.variable = "NA";}
   if (data.variable != "NA") {
   data.variable = data.variable.split("==")[0];
   data.variable = data.variable.split(">=")[0];
   data.variable = data.variable.split("<=")[0];
   data.variable = data.variable.split(">")[0];
   data.variable = data.variable.split("<")[0];
   data.variable = data.variable.split("!=")[0];
   data.variable = data.variable.split("=")[0];
   data.variableGroup = data.variable.split("_")[0];
   data.varArray1 = data.variable.split("_")[1]
      if (data.variable.split("_").length > 1) data.varArray2 = data.variable.split("_")[2];
      if (data.variable.split("_").length  > 2) data.isInt = data.variable.split("_")[3];

  if (data.variableGroup =="X" & data.varArray1 == "INTRODUCED") {
    data.isIntroduced = 1;
    data.variableGroup = "X_INTRODUCED";
    data.varArray1 = data.varArray2;
    data.varArray2 = null;
    }
  if (data.variable.slice(data.variable.length-2, data.variable.length) == "_i")
  { data.isInt = 1;
    data.variable =  data.variable.slice(0, data.variable.length-2);
  }
  else data.isInt = 0;
  }
}

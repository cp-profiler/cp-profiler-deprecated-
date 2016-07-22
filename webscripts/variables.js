var maxes = [];
var totalNodes;
var totalTime;
var totalFails;
var totalSolutions;
var projectName = "chain";
var varType = [];
var arraySize =[];

var varSum;

var rawData;

var variableListString;

function variableColour(x) {
    return "red";
}

function canonicaliseIntBool(data) {
    for (var i = 0 ; i < data.length ; i++) {
        logJ(data[i].variable);
        logJ(data[i].variable.substr(-2));
//        if (data[i].variable.substr(-2) == "_i")
    }
}

function drawVariables() {
    getData(function(data, varListString) {
        rawData = data;

        // canonicaliseIntBool(data);

        variableListString = varListString;
        categoriseVariables(variableListString);
        console.log("arrayNames = " + JSON.stringify(arrayNames));
        console.log("bareNames = " + JSON.stringify(bareNames));
        console.log("allVars = " + JSON.stringify(allVars).substr(0,300) + " ...");

        totalNodes = data.length;
        totalTime = d3.max(data, function(d) { return d.timestamp; });
        totalFails = d3.sum(data, function(d) { return d.status == 1;});
        totalSolutions = d3.sum(data, function(d) { return d.status == 0;});

        var restarts = aggregateData(data, "restartId", null);
        var varGroupSum = aggregateData(data, "variableGroup", null);
        varSum = aggregateData(data, "variable", null);
        var varPerRestart = aggregateData(data, "variable", "restartId");

        // data.forEach(function(x) {
        //     logJ(x.variableGroup);
        // });
        logJ(Object.keys(varGroupSum));

        var svgVars = d3.select("#vis1").append("svg")
            .attr("width", 500)
            .attr("height", 500);

        var infoText = svgVars.append("text")
            .attr("x", 15)
            .attr("y", 15);

        var nextY = 40;
        for (arrayKey in arrayNames) {
            var a = arrayNames[arrayKey];
            var relevantVars = [];
            var maxCount = undefined;
            for (var i = 0 ; i < allVars.length ; i++) {
                if (allVars[i].arrayName == arrayKey) {
                    relevantVars.push(allVars[i]);
                    var thisVarCount;
                    if (varSum[allVars[i].fullName] == undefined)
                        thisVarCount = 0;
                    else
                        thisVarCount = varSum[allVars[i].fullName].countNodes;
                    if (maxCount == undefined || thisVarCount > maxCount)
                        maxCount = thisVarCount;
                }
            }
            var mouseoverGroup = function(k) {
                return function() {
                    var relevantNodes = [];
                    for (var i = 0 ; i < rawData.length ; i++) {
                        var node = rawData[i];
                        if (node.variableGroup == k)
                            relevantNodes.push(node.gid);
                    }
                    infoText.text(k);
                    window.profiler.messageMany(relevantNodes);
                }
            }
            svgVars.append("rect")
                .attr("width", 15)
                .attr("height", 15)
                .style("fill", variableColour(arrayKey))
                .style("stroke", "#111111")
                .attr("x", 15)
                .attr("y", nextY)
                .on("mouseover", mouseoverGroup(arrayKey));
            var thisKeyCount;
            if (varGroupSum[arrayKey] == undefined)
                thisKeyCount = 0;
            else
                thisKeyCount = varGroupSum[arrayKey].countNodes;
            svgVars.append("text")
                .text(arrayKey + " (" + thisKeyCount + ")")
                .attr("x", 40)
                .attr("y", nextY + 7)
                .attr("alignment-baseline", "middle");
            var mouseoverFunc = function(e) {
                return function (d) {
                    var relevantNodes = [];
                    for (var i = 0 ; i < rawData.length ; i++) {
                        var node = rawData[i];
                        if (node.variable == e.fullName)
                            relevantNodes.push(node.gid);
                    }
                    var stats = varSum[e.fullName];
                    infoText.text(e.fullName + " (" + (stats == undefined ? 0 : stats.countNodes) + ")" );
                    window.profiler.messageMany(relevantNodes);
                }
            };
            var colorORaw =
                d3.scale.quantize()
                .domain([1, maxCount])
                .range(colorbrewer.Oranges[7]);
            var colorO = function(x) {
                if (x == 0) return colorbrewer.Oranges[7][0];
                else        return colorORaw(x); }
            var nextX = 150;
            var drawVarBox = function(indices) {
                var element = findElement(arrayKey, indices, relevantVars);
                if (element != undefined) {
                    logJ(element.fullName);
                    var stats = varSum[element.fullName];
                    var thisNodes;
                    if (stats == undefined)
                        thisNodes = 0;
                    else
                        thisNodes = stats.countNodes;
                    svgVars.append("rect")
                        .attr("width", 15)
                        .attr("height", 15)
                        .attr("x", nextX)
                        .attr("y", nextY)
                        .style("fill", colorO(thisNodes))
                        .style("stroke", "#111111")
                        .on("mouseover", mouseoverFunc(element));
                    nextX += 22;
                }
            }
            switch (a.dimensions.length) {
            case 1:
                for (var i = a.dimensions[0].min ; i <= a.dimensions[0].max ; i++) {
                    drawVarBox([i]);
                }
                nextY += 25;
                break;
            case 2:
                for (var i = a.dimensions[0].min ; i <= a.dimensions[0].max ; i++) {
                    for (var j = a.dimensions[1].min ; j <= a.dimensions[1].max ; j++) {
                        drawVarBox([i,j]);
                    }
                    nextY += 25;
                    if (nextX > svgVars.node().getBoundingClientRect().width)
                        svgVars.style("width", nextX);
                    nextX = 150;
                }
                nextY += 25;
                break;
            default:
                break;
            }
            if (nextY > svgVars.node().getBoundingClientRect().height)
                svgVars.style("height", nextY);
        }

        // drawVariables2(varPerRestart, varGroupSum);
    });
}

function findElement(arrayKey, indices, vars) {
    for (var i = 0 ; i < vars.length ; i++) {
        var v = vars[i];
        if (v.arrayName == arrayKey && arraysEqual(v.arrayIndices, indices))
            return v;
    }
    return undefined;
}

function arraysEqual(a, b) {
  if (a === b) return true;
  if (a == null || b == null) return false;
  if (a.length != b.length) return false;
  for (var i = 0; i < a.length; ++i) {
    if (a[i] !== b[i]) return false;
  }
  return true;
}

var arrayNames;
var bareNames;
var allVars;

function log(s) {
    console.log(s);
}

function logJ(s) {
    console.log(JSON.stringify(s));
}

function categoriseVariables(variableListString) {
    allVars = [];
    blobs = variableListString.split(";");
    for (var t = 0 ; t < 2 ; t++) {
        log("trying variable type " + t);
        // No variables of this type.
        if (blobs[t] == undefined)
            continue;
        variables = blobs[t].split(" ");
        log("split into " + variables.length + " variables of this type");
        for (var i = 0 ; i < variables.length ; i++) {
            if (t == 0) type = "int";
            if (t == 1) type = "bool";
            var variable = variables[i];
            if (variable == "ASSIGNED_AT_ROOT")
                continue;
            if (variable == " " || variable == "")
                continue;
            if (variable.substr(0, 13) == "X_INTRODUCED_")
                continue;
            parts = variable.split("_");
            if (parts[parts.length-1] == "i") {
                parts.pop();
                type = "bool";
                variable = variable.substr(0, variable.length-2);
            }
            dimensions = [];
            for (d = parts.length-1 ; d > 0 ; d--) {
                if (/^([0-9]+)/.test(parts[d])) {
                    dimensions.push(parseInt(parts[d]));
                } else {
                    break;
                }
            }
            dimensions.reverse();
            arrayElement = false;
            if (dimensions.length > 0 && dimensions.length == parts.length - 1)
                arrayElement = true;
            //console.log(type + " " + variable + " " + dimensions + " " + arrayElement);

            if (arrayElement) {
                allVars.push({ "arrayElement": true,
                            "type": type,
                            "arrayName": parts[0],
                            "fullName": variable,
                            "arrayIndices": dimensions });
            } else {
                allVars.push({ "arrayElement": false,
                            "type": type,
                            "name": variable });
            }
        }
    }

    /* for (var i = 0 ; i < allVars.length ; i++) { */
    /*     console.log(JSON.stringify(allVars[i])); */
    /* } */

    arrayNames = {};
    for (var i = 0 ; i < allVars.length ; i++) {
        v = allVars[i];
        if (v.arrayName) {
            //            arrayNames[v.arrayName] = true;
            for (d = 0 ; d < v.arrayIndices.length ; d++) {
                if (arrayNames[v.arrayName] == undefined)
                    arrayNames[v.arrayName] = { "dimensions": [],
                                                "type": v.type };
                if (arrayNames[v.arrayName].dimensions[d] == undefined)
                    arrayNames[v.arrayName].dimensions[d] = { "min": v.arrayIndices[d],
                                                              "max": v.arrayIndices[d] };
                else {
                    arrayNames[v.arrayName].dimensions[d] =
                        { "min": Math.min(arrayNames[v.arrayName].dimensions[d].min, v.arrayIndices[d]),
                          "max": Math.max(arrayNames[v.arrayName].dimensions[d].max, v.arrayIndices[d]) };
                }
            }
        }
    }

    bareNames = {};
    for (var i = 0 ; i < allVars.length ; i++) {
        v = allVars[i];
        if (!v.arrayElement) {
            bareNames[v.name] = { "type": v.type };
        }
    }
}

function drawVariables2(varPerRestart, varGroupSum) {
  var arrays = 3; // this should be automatic - in fact not really used properly.. - fix needed
  var padding = 4; // between the groups..
  var cSize = 15; // size of the coloured cell in the var group - and ideally for the others..
  varGroupNames = Object.keys(varGroupSum);
  varGroupNames.sort();


  //hard code hack -- get the types the variables and the array size for each variable group --- this should be auto...
    for (var v = 0; v < varGroupNames.length; v++) {
      if (projectName == "chain"){
          if (varGroupNames[v] == "X_INTRODUCED") {varType.push("mix"); arraySize.push([0])}
          else if (varGroupNames[v]  == "x") {varType.push("array"); arraySize.push([23]);}
          else if (varGroupNames[v] == "a" | varGroupNames[v]  == "b") {varType.push("array"); arraySize.push([24]);}
          else {varType.push("int"); arraySize.push([0]);}
          }
      if (projectName == "power"){
        if (varGroupNames[v] == "X_INTRODUCED" | varGroupNames[v] == "UNKNOWN_LITERAL") {varType.push("mix"); arraySize.push([0,0])}
        else if (varGroupNames[v]  == "module") {varType.push("array"); arraySize.push([nm, nm+modLimit]);}
        else if (varGroupNames[v] == "apgedge"| varGroupNames[v] == "mcontains" | varGroupNames[v] == "mcrossings" |
                varGroupNames[v] == "pmvedge" | varGroupNames[v] == "ppgedge") {varType.push("array"); arraySize.push([nm+modLimit, nm+modLimit]);}
        else {varType.push("int"); arraySize.push([0,0]);}
        }
        if (projectName == "vr"){
          if (varGroupNames[v] == "X_INTRODUCED" | varGroupNames[v] == "UNKNOWN_LITERAL") {varType.push("mix"); arraySize.push([0,0])}
          else if (varGroupNames[v] == "s" || varGroupNames[v] == "st"  || varGroupNames[v] == "at"  || varGroupNames[v] == "at"
         || varGroupNames[v] == "p" || varGroupNames[v] == "h"  || varGroupNames[v] == "v") {varType.push("array"); arraySize.push([V]);}
         else if (varGroupNames[v] == "sc") {varType.push("array"); arraySize.push([n]);}
          else {varType.push("int"); arraySize.push([0,0]);}
          }
      }

  var h = (cSize * varGroupNames.length) + (padding* varGroupNames.length) + 30; // bit rubbish... could do this better...
  var w = h; // for the inital list.. but this could be set better. (esp if long list..)

  var svgvariables = d3.select("#vis1").append("svg")
  .attr("class", "svgvariables")
  .attr("width", w + w)
  .attr("height", h)

    svgvariables.append("text") //  text for title of legend
    .attr("class", "varGroups")
    .attr("x", 0)
    .attr("y", 10)
    .text("Variables (count)");

    for (var j = 0; j < varGroupNames.length; j ++) // for each group create square, get data,
    {
      var variableNum = j;
      var varName = varGroupNames[variableNum]; // get each name from list..
      var count = varGroupSum[varName].countNodes; // get counts
      var failures = varGroupSum[varName].sumFails; // get count of when failed on this node.. we still need the count by nogood..
      var data = [count, failures]
      var rect =  svgvariables.append("rect")
        .data(data)
        .attr("class", "gVar " + varName)
        .attr("x", 0)
        .attr("y", cSize+(cSize*j)+(padding*j))
        .attr("vg", varName)
        .attr("width", cSize)
        .attr("height", cSize)
        .style("fill", colorVG(varName))
        // .on("mouseover", hoverover)
        // .on("mouseout", hoverout)
        .on("mouseover", function(d) {
            //clicked(d3.select(this).attr("class"), "vg");
            var g = d3.select(this).attr("vg");
            // console.warn("click on " + g);
            var relevantNodes = [];
            for (var i = 0 ; i < rawData.length ; i++) {
                var node = rawData[i];
                if (node.variableGroup == g)
                    relevantNodes.push(node.gid);
            }
            window.profiler.messageMany(relevantNodes);
        });

    svgvariables.append("text")
      .attr("x", cSize+padding)
      .attr("y", 28+(cSize*j) +(padding*j)) // could be better - find size of text for first value..
      .text(varName + " " + varType[variableNum] + " (" + varGroupSum[varName].countNodes +")")// + "(" + varGroupSum[varName].percFailedNodes + "))")
        if (varType[variableNum] == "array")
        {
        drawArray(variableNum, varName, cSize, w, h); // if it is an array draw the rest...
        }
     }
}

function drawArray(variableNum, varName, cSize, w, h) {
    var rows, cols;
    var arrayLength = arraySize[variableNum].length;

    if (arrayLength == 1)
    {
     rows = 1;
     cols = arraySize[variableNum][0];
     h = cSize * 3;
    }
    else
    {
     rows = arraySize[variableNum][0];
     cols = arraySize[variableNum][1];
    }

   var svgvariables = d3.select("#vis2").append("svg")
   .attr("class", "svgvariables")
   .attr("width", w +10)
   .attr("height", h + 30)
   .attr("transform", "translate(" + (margin.left) + "," + margin.top + ")");
  //  .attr("width", (width/(arrays+2) - padding*(arrays+2)) // cols*cSize)
  //  .attr("height",  (cSize * varGroupNames.length) + (padding* varGroupNames.length) + 20) //   rows*cSize)
  //


    var matrix = [];
    var xScale, yScale, gridMax;

    if (cols >= rows) gridMax = cols;
    else gridMax = rows;

    var xScale = d3.scale.ordinal().rangeBands([0, w],0.1).domain(d3.range(gridMax))
    var yScale = d3.scale.ordinal().rangeBands([0, h],0.1).domain(d3.range(gridMax));
    matrix = buildMatrix(rows, cols, gridMax, varName, arrayLength);
    //console.log(matrix)


    colorO = d3.scale.quantize()
           .domain([1, maxes[maxes.length-1]])
           .range(colorbrewer.Oranges[7]);

       svgvariables.append("text")
       .attr("class", "varGroups")
       .attr("x", 0)
       .attr("y", 10)
       .text(varName + "[" + arraySize[variableNum] + "]" + " (max:"+ maxes[maxes.length-1]+")")


    var row = svgvariables.selectAll(".row")
      .data(matrix)
      .enter().append("g")
      .attr("class", function(d,i) {return varName})
      .attr("transform", "translate(0, 20)")
      var cell = row.selectAll("cell")
      .data(function (d) { return d; })
      .enter()
      cell.append("rect")
      .attr("class",  function(d,i) {if (d.count == 0) {
        if (arrayLength != 1) return "cell " + varName + " " + varName + ("_" + (d.row)) + "_" + (i+1) + " " + "z0";
        else return "cell " + varName + " " + varName + ("_" + (d.col)) + " " + "z0";
      } else {
      if (arrayLength != 1) return "cell " + varName + " " + varName + ("_" + (d.row)) + "_" + (i+1);
      else return "cell " + varName + " " + varName + ("_" + (d.col));
      };})
      // --- CAN ADD A filter TO SVG COMMAND.... e.g. filter(function(d) { return d.x < 400 } and style diffrently..)
      .attr("x", function(d) { return xScale(d.x); })
      .attr("y", function(d) { return yScale(d.y); })
      .attr("width", xScale.rangeBand())
      .attr("height", function(d) {if (arrayLength == 1) return cSize; else return yScale.rangeBand();})
      .style("fill", function(d) {
        // add a hack in to remove those which are not expected to be searched
        if ( (projectName == "chain" && (d.col >= gridMax | (varName == "x" && (d.col == 1 | d.col >= gridMax-1 ))))  |
                                          (projectName == "power" && ((varName == "mcrossings" && d.col >= d.row) |
                                          (varName == "module" && d.col <= nm))))
                                          return "white";
                                          else
                                           if (d.count == 0) return "#dfdddd";
                                        else return colorO(d.count);})
      .style("stroke", function(d) {if ( (projectName == "chain" && (d.col >= gridMax | (varName == "x" && (d.col == 1 | d.col >= gridMax-1 ))))  |
                                        (projectName == "power" && ((varName == "mcrossings" && d.col >= d.row) |
                                        (varName == "module" && d.col <= nm))))
                                       return "#dfdddd";})
  //  .style("fill", function(d) {if (d.failures == 0) return "#dfdddd"; else return colorO(d.failures);})
      .on("mouseover", hoverover)
      .on("mouseout", hoverout)
      .on("click", function(d) { clicked(d3.select(this).attr("class"), "v"); });
  }


  function buildMatrix(rows, cols, gridMax, varName, arrayLength)
  {

    var matrixarray = [];
    for (var c = 0; c < rows; c ++)
    {
      // add each row to the empty array
      matrixarray.push(d3.range(cols));
      // now for each of the outer array (column) i add an col index and the x/y for each row
      matrixarray.forEach(function(d, i)
      {
        matrixarray[i].col = i

        matrixarray[i] = d3.range(cols).map(function(j) {
          var dataName;
          if (arrayLength != 1) {
            dataName =  varName + "_" + (i+1) + "_" + (j+1);
          }  else {
            dataName  =  varName + "_" + (j+1);
          }
          var  total, fails;

          if(dataName in varSum)
          {
//            if (headerNode == false) // is there a fake node added to join the restarts or not
            if (false) // is there a fake node added to join the restarts or not
            {
                if (varPerRestart[dataName][restartRequest])
                {
                total = varPerRestart[dataName][restartRequest].countNodes;
                totalLeft = varPerRestart[dataName][restartRequest].countLeftNodes;
                fails = varPerRestart[dataName][restartRequest].sumFails;
                }
                else
                {
                  total = 0;
                  totalLeft = 0;
                  fails = 0;
                }
            }
            else {
            total = varSum[dataName].countNodes;
            totalLeft = varSum[dataName].countLeftNodes;
            fails = varSum[dataName].sumFails;
            }
         }
          else {
            total = 0;
            totalLeft = 0;
            fails = 0;
          }
          return {col: j+1, row: i+1, x: j, y: i, count:total, countLefts: totalLeft, failures: fails, toSearch: true}});
        });
      };
      var lowest = Number.POSITIVE_INFINITY;
      var highest = Number.NEGATIVE_INFINITY;

      var tmp;
      for (var c=cols-1; c>=0; c--) {
        for (var r=rows-1; r>=0; r--) {
          tmp = matrixarray[r][c].count;
          if (tmp < lowest) lowest = tmp;
          if (tmp > highest) highest = tmp;
        }
      }
      maxes.push(highest); // find the maximum
    return matrixarray;
}


function hoverover() {
  var fullclass = d3.select(this).attr("class");
  var subclass = fullclass.split(" ");
  if (subclass.length >= 3)
  {
    highlight(subclass[1], "var", subclass[2]);
    d3.select(this).append("title")
    .text(function(d) {return subclass[2] + ", count: " +d.count + ", lefts: " + +d.countLefts + " leaves: " +d.failures } );
    }
  else {
    highlight(subclass[1],"varGroup");
    d3.select(this).append("title")
    .text(function(d) {return subclass[1] + ", count: " +d.count + ", lefts: " + +d.countLefts + " leaves: " +d.failures } );
    }
}
//
// function clicked() {
//   var fullclass = d3.select(this).attr("class");
//   var subclass = fullclass.split(" ");
//   if (subclass.length >= 3)
//   {
//     highlight(subclass[1], "var", subclass[2]);
//     d3.select(this).append("title")
//     .text(function(d) {return subclass[2] + ", count: " +d.count + ", lefts: " + +d.countLefts + " leaves: " +d.failures } );
//     }
//   else {
//     highlight(subclass[1],"varGroup");
//     d3.select(this).append("title")
//     .text(function(d) {return subclass[1] + ", count: " +d.count + ", lefts: " + +d.countLefts + " leaves: " +d.failures } );
//     }
// }


function hoverout() {
  highlight(null, "off");
}






function aggregateData(data, group, group2) {

  var grouping;
  if (group2 == null) {
    grouping = d3.nest()
      .key(function(d) {return d[group]})
        .sortKeys(d3.ascending)
        .rollup(function(d) {return {
                countNodes: d.length,
                countLeftNodes: d.length - d3.sum(d,function(g) {return +g.alternative == 1;}),
                percNodes: (d.length/totalNodes)*100,
                avgDepth: d3.mean(d,function(g) {return +g.depth;}),
                avgTime: d3.mean(d,function(g) {return +g.timetaken;}),
                sumTime: d3.sum(d,function(g) {return +g.timetaken;}),
                percTime: (d3.sum(d,function(g) {return +g.timetaken;})/totalTime)*100,
                sumFails: d3.sum(d,function(g) {return +g.status == 1;}),
                percFails: (d3.sum(d,function(g) {return +g.status == 1;})/totalFails)*100,
                percFailedNodes: (d3.sum(d,function(g) {return +g.status == 1;})/d.length)*100
                };
              })
        .map(data.filter(function(d) { return d.variable != "NA" && d.variable != undefined}))
          //.filter(function(d) { return d.variable != null & d.variable != "NA"}));
  } //.filter(function(d) { return d.variable != "NA" & d.variable != null; }));
  else {
    grouping = d3.nest()
    .key(function(d) {return d[group]})
      .sortKeys(d3.ascending)
      .key(function(d) {return d[group2]})
        .sortKeys(d3.ascending)
      .rollup(function(d) {return {
              countNodes: d.length,
              countLeftNodes: d.length - d3.sum(d,function(g) {return +g.alternative == 1;}),
              percNodes: (d.length/totalNodes)*100,
              avgDepth:d3.mean(d,function(g) {return +g.depth;}),
              avgTime:d3.mean(d,function(g) {return +g.timetaken;}),
              sumTime:d3.sum(d,function(g) {return +g.timetaken;}),
              percTime: (d3.sum(d,function(g) {return +g.timetaken;})/totalTime)*100,
              sumFails: d3.sum(d,function(g) {return +g.status == 1;}),
              percFails: (d3.sum(d,function(g) {return +g.status == 1;})/totalFails)*100,
              percFailedNodes: (d3.sum(d,function(g) {return +g.status == 1;})/d.length)*100
              };
            })
      .map(data.filter(function(d) { return d.variable != "NA" && d.variable != undefined}))
  }
  return grouping;
  }

function selectMany(gids) {}
function select(gid) {}

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

function drawVariables() {
    getData(function(data) {
        rawData = data;
        totalNodes = data.length;
        totalTime = d3.max(data, function(d) { return d.timestamp; });
        totalFails = d3.sum(data, function(d) { return d.status == 1;});
        totalSolutions = d3.sum(data, function(d) { return d.status == 0;});
        var restarts = aggregateData(data, "restartId", null);
        var varGroupSum = aggregateData(data, "variableGroup", null);
        varSum = aggregateData(data, "variable", null);
        var varPerRestart = aggregateData(data, "variable", "restartId");
        drawVariables2(varPerRestart, varGroupSum);
    });
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
            for (node of rawData) {
                if (node.variableGroup == g)
                    relevantNodes.push(node.gid);
            }
            // console.warn(relevantNodes);
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
            if (headerNode == false) // is there a fake node added to join the restarts or not
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
        .map(data.filter(function(d) { return d.alternative==0 && d.variable != "NA" && d.variable != undefined}))
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

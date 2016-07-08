function drawIcicle() {
    getData(function(data) {
        drawIcicle2(data);
    });
}

var nodeid2element = {};

var currentlyHighlighted = [];

function drawIcicle2(data) {
    var treeData = nestData(data)[0];

  // var x = d3.scale.linear().range([0, treeWidth]);
  // var y = d3.scale.linear().range([0, treeHeight]);
  //

  var partition = d3.layout.partition()
      .size([treeWidth, treeHeight])
      .sort(function (a, b) { return d3.ascending(a.id, b.id); })
//      .value(function(d) { return d.subtreeDepth;});
//      .value(function(d) { return 1 + 1000 * ((d.id % 2 == 0) ? 1 : 0); })
      .value(function(d) { return 1; })

  var partNodes = partition.nodes(treeData);

      // var yAxis = d3.svg.axis()
      //     .scale(xScale)
      //     .orient("left")
      //     .tickValues([10, 20, 30, 40]);
      //
      // var svgIcicle =  d3.select("#vis3").append("svg")
      //     .attr("class", "axis")
      //     .attr("width", 30)
      //     .attr("height", treeHeight)
      //   .append("g")
      //     .attr("transform", "translate(0, 0)")
      //     .call(yAxis);

  var svgIcicle = d3.select("#vis1").append("svg")
      .attr("class", "svgIcicle")
      .attr("width", treeWidth + (margin.left + margin.right))
      .attr("height", treeHeight + (margin.top + margin.bottom))
      .attr("transform", "translate(" + margin.left + "," + (margin.top) + ")");


 var rect = svgIcicle.selectAll("rect")
            .data(partition.nodes(treeData))
            .enter().append("g")
            .attr("class", function(d) { return "ice " + "r" + d.restartId + " " + d.variableGroup + " " + d.variable + " g" + d.gid + " ng" + d.visId_t; })
           //.attr("display", function(d) { return d.parent ? null : "none"; }) // hide inner ring
           .attr("display", function(d) { if (d.id <= -1) return display ="none" }) // hide inner ring

        rect.append("rect")
            .attr("x", function(d) {
                // if (d.id == 120)
                //     throw "throw";
                return d.x;
            })
            .attr("y", function(d) { return d.y;})
            .attr("width", function(d) { return d.dx; })
            .attr("height", function(d) { return d.dy; })
        .each(function(d,i) { nodeid2element[d.gid] = this; })
            // .attr("fill", function(d) { return colorVG(d.variableGroup, d.parentId, d.status); })
            // .attr("stroke", function(d) { return "yellow"; })
            // .attr("fill", function(d) { 
            //     if (d.alternative > 0 && false)
            //         return "none";
            //     else
            //         return colorVG(d.variableGroup, d.parentId, d.status); })
            // .attr("fill", function(d) { return colorStatus(d.status); })
            // .attr("fill", function(d) {
            //     if (d.status == 2)
            //         return "#f7f7f7";
            //     else
            //         return colorBackjump(d.backjumpDistance); })

//        rect.on("click", function(d) { clicked(d);});
        rect.on("mouseover", function(d) {
        //    highlight(d, "node");
               window.profiler.message(d.gid);
        })
            // .on("mouseout", function(d) {highlight(d, "off"); })
            .on("click", function(d) {
//                clicked(d, "p");
                window.profiler.messageMany([d.gid, d.gid+1]);
            }); // want to choose children get type working..
        rect.append("title") // add label to all them for the hoverover
                  .text(function(d, i) {
                    if (d.status == 0) return d.id + ": " + d.label + ". Solution: " + d.solutionString;
                    else if (d.nogoodLength < 1) return d.id + ": " + d.label;
                    else return d.id + ": " + d.label + ". Nogood Learnt: " + d.nogoodString;
                   });


    //  var lines = svgIcicle.selectAll("line")
    //        .data(partition.nodes(data))
    //        .enter().append("g")
    //  lines.append("line")
    //      .attr("x", 0)
    //      .attr("y", 10)
    //      .attr("x2", treeWidth)
    //      .attr("y2", 10)
    //      .attr("stroke", "black")
    //      .attr("stroke-width", 2)


// function clicked(d) {
//    x.domain([d.x, d.x + d.dx]);
//    y.domain([d.y, 1]).range([d.y ? 20 : 0, height]);
//
//    rect.transition()
//        .duration(750)
//        .attr("x", function(d) { return x(d.x); })
//        .attr("y", function(d) { return y(d.y); })
//        .attr("width", function(d) { return x(d.x + d.dx) - x(d.x); })
//        .attr("height", function(d) { return y(d.y + d.dy) - y(d.y); });
//  }

}

function select(gid) {
    for (var i = 0 ; i < currentlyHighlighted.length ; i++)
        d3.select(currentlyHighlighted[i]).style("fill", "#dfdddd");
    currentlyHighlighted = [nodeid2element[gid]];
    d3.select(currentlyHighlighted[0]).style("fill", "blue");
}

function selectMany(gids) {
    for (var i = 0 ; i < currentlyHighlighted.length ; i++)
        d3.select(currentlyHighlighted[i]).style("fill", "#dfdddd");
    currentlyHighlighted = [];
    for (var i = 0 ; i < gids.length ; i++) {
        var gid = gids[i];
        var node = nodeid2element[gid];
        currentlyHighlighted.push(node);
        d3.select(node).style("fill", "blue");
    }
}

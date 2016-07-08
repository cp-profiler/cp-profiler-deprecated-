// sunburst diagram - i.e. circular iceicle plot..

var nodeid2element = {};

var currentlyHighlighted = [];

function drawSunburst() {
    getData(function(data) {
        drawSunburst2(data);
    });
}

function drawSunburst2(data) {
    var treeData = nestData(data)[0];

    var formatNumber = d3.format(",d");
    var radius = (Math.min(treeWidth, treeHeight) / 2);

    var color = d3.scale.category20c();
    var x = d3.scale.linear().range([0, 2 * Math.PI]);
    var y = d3.scale.sqrt().range([0, radius]);
    var partition = d3.layout.partition()
       //.size([2 * Math.PI, radius * radius])
      .sort(function (a, b) { return d3.ascending(a.id, b.id); })
      .value(function(d) { return d.subtreeSize; });

    var nodes = partition(treeData);
    var arc = d3.svg.arc()
      .startAngle(function(d) { return Math.max(0, Math.min(2 * Math.PI, x(d.x))); })
      .endAngle(function(d) { return Math.max(0, Math.min(2 * Math.PI, x(d.x + d.dx))); })
      .innerRadius(function(d) { return Math.max(0, y(d.y)); })
      .outerRadius(function(d) { return Math.max(0, y(d.y + d.dy)); });

var size = Math.min(treeWidth, treeHeight);
   var svgSunburst = d3.select("#vis1").append("svg")
      .attr("class", "sunburst")
      .attr("width", size + (margin.left + margin.right))
      .attr("height", size + (margin.top + margin.bottom))
      .append("g")
      .attr("transform", "translate(" + size  / 2 + "," + size / 2 + ")");

    var path = svgSunburst.selectAll("path")
       .data(nodes)
       .enter().append("g")
       .each(function(d,i) { nodeid2element[d.gid] = this; })
       .attr("class", function(d) { return "ice r" + d.restartId + " " +d.variableGroup + " " + d.variable + " " + "g" + d.gid + " ng" + d.visId_t; });
       path.append("path")
          .attr("d", arc)
          .attr("display", function(d) { if (d.id <= -1) return display ="none" }) // hide inner ring

         //.attr("fill", function(d) { return colorVG(d.variableGroup, d.parentId, d.status); })

       path.on("click", function(d) { window.profiler.message(d.gid);});
       path.on("mouseover", function(d) { window.profiler.message(d.gid);});
      // path.on("click", function(d) { clicked(d, "n"); })
      // //.on("click", click)
      //     .on("mouseover", function(d) { highlight(d, "node"); })
      //     .on("mouseout", function(d) { highlight(null, "off"); });

      path.append("title")
          .text(function(d) {return d.id + ":" + d.label;});

  ///// zoom in to the subtree and show this instead...
  //   function click(d) {
  //     svgSunburst.transition()
  //         .duration(750)
  //         .tween("scale", function() {
  //           var xd = d3.interpolate(x.domain(), [d.x, d.x + d.dx]),
  //               yd = d3.interpolate(y.domain(), [d.y, 1]),
  //               yr = d3.interpolate(y.range(), [d.y ? 10 : 0, radius]);
  //           return function(t) { x.domain(xd(t)); y.domain(yd(t)).range(yr(t)); };
  //         })
  //       .selectAll("path")
  //       .attrTween("d", function(d) { return function() { return arc(d); }; });
  // }


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

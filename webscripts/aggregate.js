function drawAggregated(data, perc) // group the nodes in the data the variable in group
{
  var ht = 10;
  var w = width;

  var totalNodes = data.length;
  var totalTime = d3.max(data, function(d) { return d.timestamp; });

  data.forEach(function(d, i)
  {
    d.visId_t = Math.floor(d.visId/(totalNodes/100)*perc); // add in vis Id by perc
  });

  var which = ["numSolutions", "percLeftNodes","numNoGoods", "futureNoGoods", "medNogoodLength", "medBackjumpDistance",  "medDepth"] //"oldNoGoods"];
  var color = ["Greys", "Greys","Greys","Greys", "Greys", "Greys", "Greys"];
//var which = ["numSolutions", "percLeftNodes","percNoGoods", "avgBackjumpDistance", "maxBackjumpDistance", "medNogoodLength", "medDL", "minDepth","medDepth", "maxDepth"];
//var color = ["Reds", "Reds","Oranges","Oranges", "Oranges", "Greens", "Purples","Blues", "Blues","Blues"];


  var  dataset = d3.nest()
    .key(function(d) {return d.visId_t})
      .rollup(function(d) {return {
              medDL: d3.median(d,function(g) {return +g.decisionLevel;}),
              maxDepth: d3.max(d,function(g) {return +g.depth;}),
              minDepth: d3.min(d,function(g) {return +g.depth;}),
              medDepth: d3.median(d,function(g) {return +g.depth;}),
              medNogoodLength: d3.median(d.filter(function(d) { return +d.status == 1}),function(g) {return +g.nogoodLength;}),
            //  avgBackjumpDistance:d3.median(d.filter(function(d) { return +d.status == 1}),function(g) {return +g.backjumpDistance;}),
              medBackjumpDistance:d3.median(d.filter(function(d) { return +d.status == 1}),function(g) {return +g.backjumpDistance;}),
            //  maxBackjumpDistance:d3.max(d.filter(function(d) { return +d.status == 1}),function(g) {return +g.backjumpDistance;}),
              numNoGoods: (d3.sum(d,function(g) {return +g.status == 1;})),
            //  percNoGoods: (d3.sum(d,function(g) {return +g.status == 1;})/d.length)*100,
              numSolutions: d3.sum(d,function(g) {return +g.status == 0;}),
              percLeftNodes: (d3.sum(d,function(g) {return +g.alternative == 0;})/d.length)*100,
              percTime: (d3.sum(d,function(g) {return +g.timetaken;})/totalTime)*100,
              futureNoGoods: (d3.sum(d,function(g) {if (!g.children && g.futureNogoods && g.futureNogoods.length > 1) return +g.futureNogoods.length;})),
            //  oldNoGoods: (d3.sum(d,function(g) {if (!g.children && g.solutionObject.nogoods && g.solutionObject.nogoods.length > 1) return +g.solutionObject.nogoods.length;}))
              };
            })
      .entries(data)

  var xScale = d3.scale.ordinal().rangeBands([0, width+margin.left],0.001).domain(d3.range(dataset.length))

  var svgTimeline = d3.select("#vis1").append("svg")
      .attr("class", "svgTimeline")
      .attr("width", w)
      .attr("height", ht*which.length+1 + margin.bottom + margin.top)
      .attr("transform", "translate(" + margin.left + ", 0  )");


var rect = svgTimeline.selectAll("g")
       .data(dataset)
       .enter().append("g")
       .attr("class", function(d, i) { return "time " + "ng" + d.key; });

  for (var v = 0; v < which.length; v++)
  {
    var values = [];
      for (var j = 0; j < dataset.length; j ++)
      {
        values.push(dataset[j].values[which[v]]);
      }
//    console.log(d3.min(values) + "," + d3.max(values))

      colorB = d3.scale.quantize()
           .domain([d3.min(values), d3.max(values)])
           .range(colorbrewer[color[v]][9]);

      rect.append("rect")
          .attr("class", function(d, i) { return "time " + "ng" + d.key + " v" + v; })
          .attr("x", function (d, i)
            { if (i == 0) x = 0;
              else  x = x+xScale.rangeBand();
              return x;
            })
          .attr("y", 0 + ht*v)
          .attr("width", xScale.rangeBand())
          .attr("height", ht)
          .style("fill", function(d, i) {if ((typeof dataset[i].values[which[v]] == "undefined")) return "white"; else return colorB(dataset[i].values[which[v]]); })
        rect.on("mouseover", hoveroverT)
            .on("mouseout", hoveroutT)
            .on("click", function(d) { clicked(d3.select(this).attr("class"), "ng"); });
    }

}

function hoveroverT() {
    var fullclass = d3.select(this).attr("class");
    var subclass = fullclass.split(" ");
    var next = parseInt(subclass[1].substring(2));
    highlight(null, "nodeGroup", subclass[1]);
}

function hoveroutT() {
  highlight(null, "off");
};

// colour settings and highlighting / interaction options

var selected;

var colorO;
function colorStatus(status) {
   if (status == 0) return "green"
   else if (status == 1) return "#e3b5a3"
//   else if (status == 2) return "#d6c8b5"
   else if (status == 2) return "#ffffff"
//   else if (status == 6) return "#fffcfc"
   else if (status == 6) return "#000000"
   else return "#dfdddd";
}

function colorBackjump(dist) {
    if      (dist <= 1 || dist === undefined) return "none";
//    else if (dist == 1) return "#cccccc";
    else if (dist == 2) return "#999999";
    else if (dist == 3) return "#666666";
    else if (dist == 4) return "#333333";
    else if (dist >= 5) return "#000000";
}

function colorVG(variableGroup, parentId, status) {
  if (variableGroup == "NA" || parentId < 0)
    return "#dfdddd";
    else if (status == 6)
          return "#fffcfc";
    else if (variableGroup == "X_INTRODUCED" | variableGroup == "UNKNOWN_LITERAL")
          return "grey";
    else if (variableGroup == "numberedges" | variableGroup == "a" | variableGroup == "sc")
        return colorbrewer.Pastel1[9][0]; // #fbb4ae - "red"
    else if (variableGroup == "module" |variableGroup == "x" | variableGroup == "s")
      return colorbrewer.Pastel1[9][1]; // blue
    else if (variableGroup == "anm" | variableGroup == "b"  |variableGroup == "st")
      return colorbrewer.Pastel1[9][2]; // "#ccebc5" green
    else if (variableGroup == "apgedge" | variableGroup == "cost" | variableGroup == "p")
      return colorbrewer.Pastel1[9][3];
    else if (variableGroup == "mcrossings" | variableGroup == "at")
      return colorbrewer.Pastel1[9][4];
    else if (variableGroup == "numbercrossings" | variableGroup == "h")
        return colorbrewer.Pastel1[9][5];
    else if (variableGroup == "goal"| variableGroup == "dist")
        return colorbrewer.Pastel1[9][7];
    else if (variableGroup == "pmvedge"| variableGroup == "v")
            return colorbrewer.Pastel1[9][6];
    else if (variableGroup == "ppgedge")
            return colorbrewer.Pastel1[9][8];
    else if (variableGroup == "disjoint")
              return "#ffcbad";
    else if (variableGroup == "mcontains")
            return "#d5d5fb";
    else if (variableGroup == "notdominatedbysrc")
            return "#cde9ee";
    else if (variableGroup == "notdominatedbydest")
            return "#dff5e6";
    else
      return "black";
}

// function colorM(variableGroup, Array1) {
//   if (variableGroup == "x"){
//   var colorM = d3.scale.linear()
//   .domain([0, 16])
//   .range([0, 16]
//     .map(d3.scale.linear()
//     .domain([0, 16])
//     .range(["#fdffad", "#e6b934"])
//     .interpolate(d3.interpolateRgb)));
//  return colorM(Array1);
//  }
// }

function colorO(min, max) {

   colorO = d3.scale.quantize()
          .domain([min, max])
          .range(colorbrewer.Oranges[9]);

}

function colorScale(nodes) {
        var minTimestamp = d3.min(nodes, function(d) { return +d.timetaken; })
        var maxTimestamp = d3.max(nodes, function(d) { return +d.timetaken; })
        timeScale = d3.scale.linear()
        .domain([minTimestamp, maxTimestamp])
        .range([minTimestamp, maxTimestamp]
          .map(d3.scale.linear()
          .domain([minTimestamp, maxTimestamp])
          .range(["#e5f5e0", "#258743"])
          .interpolate(d3.interpolateRgb)));
      }


  var nogoodLinks = [];
      function highlight(d, type, subset)
      {
        if (type == "node") {
          if (typeof d.solutionObject != "undefined" && d.solutionObject["nogoods"] && d.solutionObject["nogoods"].length > 0)
           { nogoodLinks = d.solutionObject["nogoods"];
           //console.log(nogoodLinks);
              for (var l in nogoodLinks) {
                var node = "n" + nogoodLinks[l];
                d3.selectAll(".ice" + "." + node).classed("nogoodHighlight", true);
                }
           }
              if (d.futureNogoods != "undefined") {
                  for (var l in d.futureNogoods) {
                      var node = "n" + d.futureNogoods[l];
                      d3.selectAll(".ice" + "." + node).classed("futureNogoodHighlight", true);
                  }
            }
        var node = "n" + d.id;
        d3.selectAll(".ice" + "." + d.variableGroup + "." + d.variable + "." + node).classed("highlight", true);
        d3.selectAll(".node" + "." + d.variableGroup + "." + d.variable + "." + node).classed("highlight", true);
        d3.selectAll(".gVar." + d.variableGroup).classed("highlight", true);
        d3.selectAll(".cell." + d.variable).classed("highlight", true);
        d3.selectAll(".cell." + d.variable + "." + "z0").classed("highlight", false);
        d3.selectAll(".time." + "ng" + d.visId_t).classed("highlight", true);
          var objDom = "";
          if (typeof d.solutionString != "undefined" && d.solutionString != "" && objectiveDomain != "" && !d.solutionObject["nogoods"])
        {
         var objMinD = d.solutionObject[objectiveDomain][0][0];
         var objMaxD = d.solutionObject[objectiveDomain][0][1];
         objDom = ": domain: [" + objMinD + ","+ objMaxD + "]";
        }
        var nogoodtext = "";
        if (d.nogoodLength > 0) {nogoodtext = "Nogood: " + d.nogoodString};
        if (nogoodtext.length > 150) { nogoodtext = nogoodtext.substring(0,150) + "... (nogoodStringLength: " + d.nogoodStringLength +")"};
        if (selected == false)  d3.select("#breadcrumb").html("node" + d.id + ": objMin(range): " + d.objDomainMin + "("+ d.objDomainRange + "). Label: " + d.label + "<br/>" + nogoodtext);
        //+ "<br/>" + nogoodtext);} // to add decision --- make this function like clicked!}
      } else if (type == "var")
        {
          d3.selectAll(".node" + "." + d + "." + subset).classed("highlight", true);
          d3.selectAll(".ice" + "." + d + "." + subset).classed("highlight", true);
          d3.selectAll(".cell."+ d + "." + subset).classed("highlight", true);
          d3.selectAll(".gVar." + d).classed("highlight", true);
        } else if (type == "varGroup")
        {

          d3.selectAll(".node." + d).classed("highlight", true);
          d3.selectAll(".ice." + d).classed("highlight", true);
          d3.selectAll(".gVar." + d).classed("highlight", true);
          d3.selectAll(".cell." + d).classed("highlight", true);
          d3.selectAll(".cell." + d + "." + "z0").classed("highlight", false);
        } else if (type == "nodeGroup")
        {
        d3.selectAll(".time." + subset).classed("highlight", true);
        d3.selectAll(".ice" + "." + subset).classed("highlight", true);
        d3.selectAll(".node" + "." + subset).classed("highlight", true);
        } else if (type == "r")
        {
           d3.selectAll(".node." + "r" + d.key).classed("highlight", true);
           d3.selectAll(".ice." + "r" + d.key).classed("highlight", true);
           d3.select("#breadcrumb").html("Selected restartId: " + d.key);
             }
       else // if (type == "off")
          {
            d3.selectAll(".ice").classed("nogoodHighlight", false);
            d3.selectAll(".ice").classed("futureNogoodHighlight", false);
            d3.selectAll(".node").classed("highlight", false);
            d3.selectAll(".cell").classed("highlight", false);
            d3.selectAll(".gVar").classed("highlight", false);
            d3.selectAll(".ice").classed("highlight", false);
            d3.selectAll(".time").classed("highlight", false);
            if (selected == false) {d3.select("#breadcrumb").html("")};
          }


      }


      function clicked(d, type) {
        // n = node, p = path, v = variable
        if (d3.selectAll(".ice").classed("selected") | d3.selectAll(".ice.selected")[0].length > 1)
        {  selected = false;
           d3.selectAll(".link").classed("selected", false);
           d3.selectAll(".node").classed("selected", false);
           d3.selectAll(".cell").classed("selected", false);
           d3.selectAll(".ice").classed("selected", false);
           d3.selectAll(".gVar").classed("selected", false);
           d3.selectAll(".time").classed("selected", false);
           d3.selectAll(".ice").classed("selected", false);
           d3.selectAll(".ice").classed("nogoodSelected", false);
           d3.selectAll(".ice").classed("futureNogoodSelected", false);
           d3.select("#breadcrumb").html("");
          return;
        }
        selected = true;
       var nogoodtext = "";
       if (type == "n"){
         if (typeof d.solutionObject != "undefined" && d.solutionObject["nogoods"] && d.solutionObject["nogoods"].length > 0)
          { nogoodLinks = d.solutionObject["nogoods"];
             //console.log(nogoodLinks);
             for (var l = 0; l < nogoodLinks.length; l++) {
               var node = "n" + nogoodLinks[l];
               d3.selectAll(".ice" + "." + node).classed("nogoodSelected", true);
               }
               if (d.futureNogoods != "undefined") {
                   for (var l in d.futureNogoods) {
                       var node = "n" + d.futureNogoods[l];
                       d3.selectAll(".ice" + "." + node).classed("futureNogoodSelected", true);
                   }
              }
           }

           d3.selectAll(".node" + "." + "n" + d.id).classed("selected", true);
           d3.selectAll(".cell." + d.variable).classed("selected", true);
           d3.selectAll(".ice" + "." + "n" + d.id).classed("selected", true);
           d3.selectAll(".time" + "." + "ng" + d.visId_t).classed("selected", true);
           d3.selectAll(".gVar." + d.variableGroup).classed("selected", true);
           if (d.nogoodLength > 0) {nogoodtext = "Nogood: " + d.nogoodString};
           if (nogoodtext.length > 150) { nogoodtext = nogoodtext.substring(0,150) + "... (nogoodStringLength: " + d.nogoodStringLength +")"};
           d3.select("#breadcrumb").html("node" + d.id + ": objMin(range): " + d.objDomainMin + "("+ d.objDomainRange + "). Label: " + d.label + "<br/>" + nogoodtext);
       }
       else if (type == "v"){
         var subclass = d.split(" ");
           d3.selectAll(".node" + "." + subclass[2]).classed("selected", true);
           d3.selectAll(".cell." + subclass[2]).classed("selected", true);
           d3.selectAll(".ice" + "." + subclass[2]).classed("selected", true);
           d3.selectAll(".gVar." + subclass[1]).classed("selected", true);
           //if (d.nogoodLength > 0) { nogoodtext = "Nogood Learnt :" + d.nogoodString};
           d3.select("#breadcrumb").html("Selected variable: " + subclass[2]);
       }
       else if (type == "vg"){
         var subclass = d.split(" ");
           d3.selectAll(".node" + "." + subclass[1]).classed("selected", true);
           d3.selectAll(".cell." + subclass[1]).classed("selected", true);
           d3.selectAll(".ice" + "." + subclass[1]).classed("selected", true);
           d3.selectAll(".gVar." + subclass[1]).classed("selected", true);
           d3.selectAll(".cell." + subclass[1] + "." + "z0").classed("selected", false);
           //if (d.nogoodLength > 0) { nogoodtext = "Nogood Learnt :" + d.nogoodString};
           d3.select("#breadcrumb").html("Selected variable group: " + subclass[1]);
       }
       else if (type == "ng"){
         var subclass = d.split(" ");
         console.log(subclass)
           d3.selectAll(".time" + "." + subclass[2]).classed("selected", true);
           d3.selectAll(".node" + "." + subclass[2]).classed("selected", true);
           d3.selectAll(".ice" + "." + subclass[2]).classed("selected", true);
           d3.select("#breadcrumb").html("Selected variable group: " + subclass[1]);
       }
       else if (type == "r"){
           d3.selectAll(".ice." + "r" + d.key).classed("selected", true);
           d3.selectAll(".node." + "r" + d.key).classed("selected", true);
           d3.selectAll(".ice." + "r" + d.key).classed("selected", true);
           d3.select("#breadcrumb").html("Selected restartId: " + d.key);
       }
        else if (type == "p") {
          var pathLength;
          var pathDecisions = [];
          var pathLabels = [];
          var pathNoGoods = [];
          var ancestors = [];
          var getParent = d;
          while (getParent != null & getParent.root != true) {
              pathDecisions.push(getParent.variable);
              pathLabels.push(" " + getParent.label);
              ancestors.push(getParent);
              if (!getParent.children) pathNoGoods.push(getParent.nogoodString); // print the no good of the leaf!
              getParent = getParent.parent;

            }
            // var nogoodtext = "";
            // if (pathNoGoods.length > 0) { nogoodtext = "Nogood Learnt :" + pathNoGoods};
            // if (pathNoGoods.length > 0) { nogoodtext = "Nogood Learnt :" + pathNoGoods};

            pathLength = ancestors.length;
            for (i = 0; i < pathLength; i++){
              highlightLinks(ancestors[i].parentId, ancestors[i].id, ancestors[i].variableGroup, ancestors[i].variable)
            }

            d3.select("#breadcrumb")
            .html("Search Decisions (selected to root): " + pathLabels);
            // + "<br/>" + nogoodtext);
          }
        else
        console.log("selection not defined");
        return;
      //             animateParentChain(matchedLinks);
        };



      function highlightLinks(parentId, id, variableGroup, variable)
      {
        var source = "s" + parentId;
        var target = "t" + id;
        var node = "n" + id;
        if (parentId == null)

        {
          if (
             true) {
          d3.selectAll(".link").classed("selected", false);
          d3.selectAll(".node").classed("selected", false);}
          d3.selectAll(".cell").classed("selected", false);
          d3.selectAll(".ice").classed("selected", false);
          d3.selectAll(".gVar").classed("selected", false);
          d3.selectAll(".time.").classed("highlight", false);
        }
        else
        {
          if (
             true) {
            d3.selectAll(".link." + source + "." + target).classed("selected", true);
            d3.selectAll(".node." + variableGroup + "." + variable + "." + node).classed("selected", true);}
            d3.selectAll(".cell." + variable).classed("selected", true);
            d3.selectAll(".ice." + variableGroup + "." + variable + "." + node).classed("selected", true);
            d3.selectAll(".gVar." + variableGroup).classed("selected", true);
        }
      }

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

// nest the data for the treeData structure
function nestData(data) {
   var restart = 0;
   var treeData = [];
   // var previoustime = 0;
   // var previousObjdomain = 0;
   // var previousObjMin = 0;
   // var dataMap = data.reduce(function(map, node) // map to parents
   // {  map[node.id] = node;
   //   return map;
   // }, {});
    var dataMap = {};
    for (var i = 0 ; i < data.length ; i++) {
        var node = data[i];
        dataMap[node.id] = node;
    }
//  var restartCount = 0;
    var futureNogoods = [];
    for (var i = 0 ; i < data.length ; i++) {
        var node = data[i];
   // data.forEach(function(node, i)
                // {
                    // add to parent
        var parent = dataMap[node.parentId];
         // console.log(parent)
         if (!node.visId) node.visId = i;

            var previousNode;
            if (node.id > 0) {
                var offset = 1;
                do {
                    previousNode = dataMap[node.id - offset];
                    offset++;
                } while (previousNode === undefined && offset <= node.id);
            }

       //   add timetaken as a variable...
          if (node.id < 0){}
          else if (node.id == 0 || previousNode === undefined)
           {
              node.timetaken = node.timestamp;
           } else {
                // console.log(node);
                node.timetaken = node.timestamp - previousNode.timestamp;
           }
        if (!node.restartId) {
            getVariables(node);
        }

            // var futureNogoods = [];
            // for (var i in data) {
                if (data[i].solutionObject != undefined && "nogoods" in data[i].solutionObject) {
                    for (j in data[i].solutionObject.nogoods) {
                        var nogoodid = data[i].solutionObject.nogoods[j];
                        var nogoodnode = dataMap[nogoodid];
                        if (futureNogoods[nogoodid] === undefined)
                            futureNogoods[nogoodid] = [];
                        futureNogoods[nogoodid].push(node.id);
                        //console.log("pushing " + node.id + " onto " + nogoodid);
                    }
                }
            // }

           if (node.solutionObject
               && typeof node.solutionObject != "undefined"
               && typeof node.solutionObject[objectiveDomain] != "undefined"
               && !node.solutionObject["nogoods"])
           {
            node.objDomainMin = (node.solutionObject[objectiveDomain][0][0]);
            node.objDomainRange = (node.solutionObject[objectiveDomain][0][1]-node.solutionObject[objectiveDomain][0][0])+1;
            // previousObjdomain = node.objDomainRange;
            // previousObjMin = node.objDomainMin;
          } else {
              if (previousNode != undefined) {
                  node.objDomainRange = previousNode.objDomainRange;
                  node.objDomainMin = previousNode.objDomainMin;
              }
          }

          var count = 0;
          if (parent || node.reparented == true) {
            if (node.parentId == -1)
                node.root = true;
//             if (node.parentId == -1 && node.id > 1 && !node.reparented) {
//                 restart++;
// //                restartCount = 0;
//             }

            if (node.parentId < 0) {
                node.restartCount = 0;
            } else if (node.parentId >= 0) {
                node.restartCount = previousNode.restartCount + 1;
            }

            // node.restartCount = restartCount;
            // restartCount = restartCount+1;
            // create child array if it doesn't exist
            (parent.children || (parent.children = []))
            // add node to child array
            .push(node);

             // if (!node.restartId) node.restartId = restart;
          } else //(the children)
          {
        //    if (node.parentId == -1 & node.id > 1) {restart++;  restartCount = 0;};
            // parent is null or missing
            // if (!node.restartId) node.restartId = restart;
            node.restartCount = 0;
            // restartCount = restartCount+1;
            treeData.push(node);
          }
        }

    for (i in futureNogoods) {
        dataMap[i].futureNogoods = futureNogoods[i];
    }

    return treeData;
 }

function showNode(node) {
    var children = "";
    if (node.children) {
        for (var i = 0 ; i < node.children.length ; i++)
            children += " " + node.children[i].id;
    }
    return node.id + ": " + children;
}

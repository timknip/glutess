var util = require('util');
var glutess = require('./build/Release/glutess');

console.log('Whee!');

var gluTess = new glutess.GluTess(3);

gluTess.callback(glutess.BEGIN, function(t) { console.log('begin:' + t);});
gluTess.callback(glutess.VERTEX, function(a) { console.log('vertex:' + util.inspect(a));});
gluTess.callback(glutess.END, function() { console.log('end');});
gluTess.callback(glutess.ERROR, function() { console.log('error');});
gluTess.callback(glutess.EDGE_FLAG, function() { console.log('edgeFlag');});
gluTess.callback(glutess.COMBINE, function(c,v,w) { console.log('combine');});

gluTess.beginPolygon();
gluTess.beginContour();
gluTess.vertex([0,0,0]);
gluTess.vertex([0,10,0]);
gluTess.vertex([10,10,0]);
gluTess.vertex([10,0,0]);
gluTess.endContour();
gluTess.beginContour();
gluTess.vertex([7,2,0]);
gluTess.vertex([7,8,0]);
gluTess.vertex([2,0,0]);
gluTess.endContour();
gluTess.endPolygon();

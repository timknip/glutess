var glutess = require('./index');

// Create a tesselator with vertexSize = 3
var gluTess = new glutess.GluTess(3);

// Push a value from 'n' back in array
var pushPrev = function (v,n) {
  v.push(v[v.length - n]);
}

var vertexType = 0;
var vertexNumber = 0;
var vertices = [];

// setup callbacks
gluTess.callback(glutess.BEGIN, function(t) {
  vertexType=t;
  vertexNumber = 0;
});
gluTess.callback(glutess.VERTEX, function(a) {
  if(vertexNumber > 2) {
    if(vertexType === glutess.TRIANGLE_STRIP) {
      if((vertexNumber-3) % 2) {
        pushPrev(vertices, 3);
        pushPrev(vertices, 2);
      } else {
        pushPrev(vertices, 1);
        pushPrev(vertices, 3);
      }
    } else if (vertexType === glutess.TRIANGLE_FAN) {
      pushPrev(verts, 3);
      pushPrev(verts, 2);
    }
  }
  vertices.push(a);
  vertexNumber += 1;
});
gluTess.callback(glutess.END, function(c) {console.log('end')});
gluTess.callback(glutess.ERROR, function() {console.log('error');});
gluTess.callback(glutess.EDGE_FLAG, function() {console.log('edge_flag');});
gluTess.callback(glutess.COMBINE, function(c,v,w,n) {
  var r = new Array(n);
  for(var i=0; i < n; ++i)
    r[i] = v[0][i]*w[0] + v[1][i]*w[1] + v[2][i]*w[2] + v[3][i]*w[3];
  return r;
});

gluTess.property(glutess.WINDING_RULE, glutess.WINDING_NEGATIVE);

// a clockwise polygon
var polyA = [
  [0,0,0],
  [0,10,0],
  [10,10,0],
  [10,0,0]
];

// a counter clockwise polygon
var polyB = [
  [5,5,0],
  [15,5,0],
  [15,15,0],
  [5,15,0]
];

var i;

gluTess.beginPolygon();
gluTess.beginContour();
for (i = 0; i < polyA.length; ++i) {
  gluTess.vertex(polyA[i]);
}
gluTess.endContour();
gluTess.beginContour();
for (i = 0; i < polyB.length; ++i) {
  gluTess.vertex(polyB[i]);
}
gluTess.endContour();
gluTess.endPolygon();

// print triangles
for (i = 0; i < vertices.length; i += 3) {
  console.log(vertices[i]);
  console.log(vertices[i+1]);
  console.log(vertices[i+2]);
  console.log('--------');
}

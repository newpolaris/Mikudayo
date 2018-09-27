xof 0303txt 0032
template Vector {
 <3d82ab5e-62da-11cf-ab39-0020af71e433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template MeshFace {
 <3d82ab5f-62da-11cf-ab39-0020af71e433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template Mesh {
 <3d82ab44-62da-11cf-ab39-0020af71e433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

template MeshNormals {
 <f6f23f43-7686-11cf-8f52-0040333594a3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template Coords2d {
 <f6f23f44-7686-11cf-8f52-0040333594a3>
 FLOAT u;
 FLOAT v;
}

template MeshTextureCoords {
 <f6f23f40-7686-11cf-8f52-0040333594a3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template ColorRGBA {
 <35ff44e0-6c7c-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template ColorRGB {
 <d3e16e81-7835-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template Material {
 <3d82ab4d-62da-11cf-ab39-0020af71e433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshMaterialList {
 <f6f23f42-7686-11cf-8f52-0040333594a3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material <3d82ab4d-62da-11cf-ab39-0020af71e433>]
}


Mesh {
 37;
 0.000000;0.015000;0.000000;,
 0.633890;0.015000;-3.594950;,
 0.000000;0.015000;-3.650410;,
 1.248510;0.015000;-3.430260;,
 1.825200;0.015000;-3.161340;,
 2.346430;0.015000;-2.796370;,
 2.796370;0.015000;-2.346430;,
 3.161340;0.015000;-1.825200;,
 3.430260;0.015000;-1.248510;,
 3.594950;0.015000;-0.633890;,
 3.650410;0.015000;0.000000;,
 3.594950;0.015000;0.633890;,
 3.430260;0.015000;1.248510;,
 3.161340;0.015000;1.825200;,
 2.796370;0.015000;2.346430;,
 2.346430;0.015000;2.796370;,
 1.825200;0.015000;3.161340;,
 1.248510;0.015000;3.430260;,
 0.633890;0.015000;3.594950;,
 0.000000;0.015000;3.650410;,
 -0.633880;0.015000;3.594950;,
 -1.248510;0.015000;3.430260;,
 -1.825200;0.015000;3.161340;,
 -2.346430;0.015000;2.796370;,
 -2.796370;0.015000;2.346430;,
 -3.161340;0.015000;1.825200;,
 -3.430260;0.015000;1.248510;,
 -3.594950;0.015000;0.633890;,
 -3.650410;0.015000;0.000000;,
 -3.594950;0.015000;-0.633880;,
 -3.430260;0.015000;-1.248510;,
 -3.161340;0.015000;-1.825200;,
 -2.796370;0.015000;-2.346430;,
 -2.346440;0.015000;-2.796370;,
 -1.825200;0.015000;-3.161340;,
 -1.248510;0.015000;-3.430260;,
 -0.633890;0.015000;-3.594950;;
 36;
 3;0,1,2;,
 3;0,3,1;,
 3;0,4,3;,
 3;0,5,4;,
 3;0,6,5;,
 3;0,7,6;,
 3;0,8,7;,
 3;0,9,8;,
 3;0,10,9;,
 3;0,11,10;,
 3;0,12,11;,
 3;0,13,12;,
 3;0,14,13;,
 3;0,15,14;,
 3;0,16,15;,
 3;0,17,16;,
 3;0,18,17;,
 3;0,19,18;,
 3;0,20,19;,
 3;0,21,20;,
 3;0,22,21;,
 3;0,23,22;,
 3;0,24,23;,
 3;0,25,24;,
 3;0,26,25;,
 3;0,27,26;,
 3;0,28,27;,
 3;0,29,28;,
 3;0,30,29;,
 3;0,31,30;,
 3;0,32,31;,
 3;0,33,32;,
 3;0,34,33;,
 3;0,35,34;,
 3;0,36,35;,
 3;0,2,36;;

 MeshNormals {
  37;
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;,
  0.000000;1.000000;0.000000;;
  36;
  3;0,1,2;,
  3;0,3,1;,
  3;0,4,3;,
  3;0,5,4;,
  3;0,6,5;,
  3;0,7,6;,
  3;0,8,7;,
  3;0,9,8;,
  3;0,10,9;,
  3;0,11,10;,
  3;0,12,11;,
  3;0,13,12;,
  3;0,14,13;,
  3;0,15,14;,
  3;0,16,15;,
  3;0,17,16;,
  3;0,18,17;,
  3;0,19,18;,
  3;0,20,19;,
  3;0,21,20;,
  3;0,22,21;,
  3;0,23,22;,
  3;0,24,23;,
  3;0,25,24;,
  3;0,26,25;,
  3;0,27,26;,
  3;0,28,27;,
  3;0,29,28;,
  3;0,30,29;,
  3;0,31,30;,
  3;0,32,31;,
  3;0,33,32;,
  3;0,34,33;,
  3;0,35,34;,
  3;0,36,35;,
  3;0,2,36;;
 }

 MeshTextureCoords {
  37;
  0.500000;0.500000;,
  0.583480;0.973470;,
  0.500000;0.980770;,
  0.664430;0.951780;,
  0.740380;0.916360;,
  0.809030;0.868290;,
  0.868290;0.809030;,
  0.916360;0.740380;,
  0.951780;0.664430;,
  0.973470;0.583480;,
  0.980770;0.500000;,
  0.973470;0.416520;,
  0.951780;0.335570;,
  0.916360;0.259620;,
  0.868290;0.190970;,
  0.809030;0.131710;,
  0.740380;0.083640;,
  0.664430;0.048220;,
  0.583480;0.026530;,
  0.500000;0.019230;,
  0.416520;0.026530;,
  0.335570;0.048220;,
  0.259620;0.083640;,
  0.190970;0.131710;,
  0.131710;0.190970;,
  0.083640;0.259620;,
  0.048230;0.335570;,
  0.026530;0.416520;,
  0.019230;0.500000;,
  0.026530;0.583480;,
  0.048220;0.664430;,
  0.083640;0.740380;,
  0.131710;0.809030;,
  0.190970;0.868290;,
  0.259620;0.916360;,
  0.335570;0.951770;,
  0.416510;0.973470;;
 }

 MeshMaterialList {
  1;
  36;
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0;

  Material {
   1.000000;1.000000;1.000000;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.000000;0.000000;0.000000;;
  }
 }
}
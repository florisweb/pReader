
$fn=25;

wallThickness = 1.5;
topThickness = 0.7;

innerScreenLeft = 3;
innerScreenTop = 4;
innerScreenWidth = 198.16;
innerScreenDepth = 279.52;
screenHeight = 1.5;

ESPDepth = 54;
ESPPortWidth = 9;
ESPPortHeight = 4;
ESPPortLeftOffset = 10;

screenWidth = 212.26;
extraSideWidth = 23;
screenMargin = .5;
width = screenWidth + extraSideWidth + wallThickness * 2 + screenMargin * 2;
depth = 286.32 + wallThickness * 2 + screenMargin * 2;
height = 10 + topThickness;


mirror([1, 0, 0])
difference() {
    frontHalf();
    cube([1000, depth / 2, 1000]);
}




module frontHalf() {
    difference() {
        mainFrameFrontHalf();
        translate([width - wallThickness - ESPPortWidth - ESPPortLeftOffset, depth - wallThickness, topThickness + 0.5])
        cube([ESPPortWidth, wallThickness, ESPPortHeight]);
    }
}

module mainFrameFrontHalf() {
    difference() {
        cube([width, depth, height]);
        translate([
            innerScreenLeft + wallThickness + screenMargin, -(innerScreenTop + wallThickness - (depth - innerScreenDepth)) + screenMargin, 0])
        cube([innerScreenWidth, innerScreenDepth, 100]);
        
        
        translate([wallThickness, wallThickness, topThickness])
        cube([width - wallThickness * 2, depth - wallThickness * 2, height]);
    }
    stopperWidth = width - screenWidth - wallThickness * 2 - screenMargin * 2;
    translate([width - stopperWidth - wallThickness, wallThickness, 0])
    cube([stopperWidth, 8, height]);
    translate([width - stopperWidth - wallThickness, depth - wallThickness - 8 - ESPDepth, 0])
    cube([stopperWidth, 8, height]);
    
//    translate([0, 0, 50])
//    triConnectionPoint();
}


module triConnectionPoint(width = 10, depth = 10) {
    boxHeight = 2;
    prismHeight = height - topThickness - screenHeight - boxHeight;
    
   // difference() 
    {
        //union() 
        {
            rotate([0, 180, 90])
            translate([0, 0, -prismHeight])
            prism(width, depth, prismHeight);
            translate([-width, -depth, prismHeight])
            cube([width, depth, boxHeight]);
        }
        
        translate([-width/2, -depth/2, -3])
        cylinder(r=3, h=30);
    }
        
    
}
    
    

module prism(width, depth, height) {
    polyhedron(
        points=[[0, 0, 0], [width, 0, 0], [0, depth, 0], [width, depth, 0], [0, depth, height],     [width, depth, height]
    ],                                 // the apex point 
        faces=[[0, 1, 2], [1, 2, 3], [0, 1, 4], [4, 1, 5], [1, 3, 5], [2, 3, 5], [2, 4, 5], [0, 2, 4]]
    );
}
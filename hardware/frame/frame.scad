
$fn=25;

wallThickness = 1.5;
topThickness = 0.7;

innerScreenLeft = 3;
innerScreenTop = 6;
innerScreenWidth = 195.16;
innerScreenDepth = 275.52;

screenWidth = 212.26;
extraSideWidth = 20;
screenMargin = 0.3;
width = screenWidth + extraSideWidth + wallThickness * 2 + screenMargin * 2;
depth = 286.32 + wallThickness * 2 + screenMargin * 2;
height = 10 + topThickness;



difference() {
    frontHalf();
    cube([1000, depth / 2, 1000]);
}

module frontHalf() {
    difference() {
        cube([width, depth, height]);
        translate([
            innerScreenLeft + wallThickness + screenMargin, -(innerScreenTop + wallThickness - (depth - innerScreenDepth)) + screenMargin, 0])
        cube([innerScreenWidth, innerScreenDepth, 100]);
        
        
        translate([wallThickness, wallThickness, topThickness])
        cube([width - wallThickness * 2, depth - wallThickness * 2, height]);
    }
    stopperWidth = width - screenWidth - wallThickness * 2;
    translate([width - stopperWidth - wallThickness, wallThickness, 0])
    cube([stopperWidth, 5, height]);
    translate([width - stopperWidth - wallThickness, depth - wallThickness - 5, 0])
    cube([stopperWidth, 5, height]);
}
    
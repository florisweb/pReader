

module boxWithRoundedCorners(width, depth, height, cornerRadius) {
    innerWidth = width - cornerRadius * 2;
    innerDepth = depth - cornerRadius * 2;
    
    translate([cornerRadius, 0, 0])
    cube([innerWidth, depth, height]);
    translate([0, cornerRadius, 0])
    cube([width, innerDepth, height]);
    
    translate([cornerRadius, cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([width - cornerRadius, cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([width - cornerRadius, depth - cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([cornerRadius, depth - cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
}


module eqTriangle(width, height) {    
    triangle_points = [
        [0, 0],  // Point A
        [width, width * sqrt(4/3) / 2], // Point B
        [0, width * sqrt(4/3)]  // Point C
    ];

    linear_extrude(height = height) {
        polygon(points = triangle_points);
    }
}






// Max compres % = (depth - 2 * segmentThickness * segments) / depth
module spring(width, depth, height, segments, segmentThickness) {
    depthPerSegment = depth / segments;
    
    for (i = [0:1:segments - 1]) 
    {
        translate([0, i * depthPerSegment, 0]) {
            cube([width, segmentThickness, height]);
            cube([segmentThickness, depthPerSegment / 2, height]);
            translate([0, depthPerSegment / 2, 0])
            cube([width, segmentThickness, height]);    
            translate([width - segmentThickness, depthPerSegment / 2, 0])    
            cube([segmentThickness, depthPerSegment / 2, height]);
        }
    }
}
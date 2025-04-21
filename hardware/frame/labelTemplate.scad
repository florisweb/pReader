




difference() {
   cube([30, 37, .7]);
    translate([2+25, 2, 0])
//   buttonLabels();
    linear_extrude(.15)
    scale([.07, .07, 1])
    rotate([0, 180, 0])
    import("/Library/WebServer/Documents/git/AutoHomeProjects/pReader/hardware/frame/readerLogo.svg");
}


module buttonLabels() {
    translate([15, 41 + 11.5, 0])
    rotate([0, 0, 180])
    eqTriangle(10, .15);

    translate([5, 24, 0])
    linear_extrude(.15)
    text("OK", size=5);


    translate([5, 0, 0])
    eqTriangle(10, .15);
}

module eqTriangle(width, height) {
//    prism(width, width * sqrt(4/3), height);
    
     triangle_points = [
        [0, 0],  // Point A
        [width, width * sqrt(4/3) / 2], // Point B
        [0, width * sqrt(4/3)]  // Point C
    ];

    linear_extrude(height = height) {
        polygon(points = triangle_points);
    }
}

